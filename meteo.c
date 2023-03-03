#define _GNU_SOURCES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    
#include <string.h>

#include <sys/shm.h>  
#include <sys/msg.h> 
#include <semaphore.h>
#include <fcntl.h>      
#include <errno.h>

#include <time.h>
#include <signal.h>

#include <math.h>

#include "lib/utilities.h"
#include "lib/config.h"
#include "lib/customMacro.h"

#include "lib/meteo.h"

const int HOUR_IN_DAY = 24;

int *configArr = 0;

int *acknowledgeInitArr = 0; /* TODO gesrie init di meteo */

Boat *boatArr = 0;
Port *portArr = 0;

int weatherStatusLength = 0;
weatherStatusHandler *weatherStatusArr = 0;

int simulationRunning = 1;
int masterPid = 0;
int weatherId = -1;

void handle_weather_simulation_signals(int signal) {

    switch (signal)
    {
        case SIGSYS: /* End simulation */
            simulationRunning = 0;
            break;
            
        default:
            handleError("Intercept a unhandled signal");
            break;
    }
}

void handle_weather_stopProcess() {

    /* Block all incoming signals after the first SIGINT */
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    debug("Stopping weather...");
    cleanup();
    exit(0);
}

/* argv[0]=configsh | argv[1]=bortsh | argv[2]=portsh | argv[3]=akish */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeSingalsMask();
    initializeEnvironment();
    initializeSingalsHandlers();

    if (initializeConfig(argv[0]) == -1) {
        handleError("Initialization of weather config failed");
        safeExit(1);
    }
    
    if (initializeWeather(argv[1], argv[2], argv[3]) == -1) {
        handleError("Initialization of weather failed");
        safeExit(2);
    }

    if (work() == -1) {
        handleError("Error during weather work");
        safeExit(3);
    }

    /* Acknowledge finish */
    setAcknowledge();

    if (cleanup() == -1) {
        handleError("weather cleanup failed");
        safeExit(4);
    }

    return 0;
}

void initializeSingalsMask() {

    sigset_t sigMask;

    sigfillset(&sigMask);
    sigdelset(&sigMask, SIGSYS);
    sigdelset(&sigMask, SIGINT);
    sigprocmask(SIG_SETMASK, &sigMask, NULL);
}

int initializeSingalsHandlers() {

    setpgid(getpid(), getppid());
    masterPid = getppid();

    signal(SIGSYS, handle_weather_simulation_signals);
    signal(SIGINT, handle_weather_stopProcess);

    return 0;
}

int initializeConfig(char *configSharedMemoryIdString) {

    char *p;
    
    int configSharedMemoryId = strtol(configSharedMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configSharedMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    return 0;
}

int initializeWeather(char *boatSharedMemoryIdS, char *portSharedMemoryIdS, char *acknowledgeDumpSharedMemoryIdS) {

    char *p;
    int i;
    int boatSharedMemoryId = strtol(boatSharedMemoryIdS, &p, 10);
    int portSharedMemoryId = strtol(portSharedMemoryIdS, &p, 10);
    int acknowledgeInitSharedMemoryId = strtol(acknowledgeDumpSharedMemoryIdS, &p, 10);
    
    boatArr = (Boat*) shmat(boatSharedMemoryId, NULL, 0);
    if (boatArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    portArr = (Port*) shmat(portSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    acknowledgeInitArr = (int*) shmat(acknowledgeInitSharedMemoryId, NULL, 0);
    if (acknowledgeInitArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    weatherStatusLength += configArr[SO_SWELL_DURATION] <= HOUR_IN_DAY ? 1 : (HOUR_IN_DAY / configArr[SO_SWELL_DURATION]) + 1;
    weatherStatusLength += configArr[SO_STORM_DURATION] <= HOUR_IN_DAY ? 1 : (HOUR_IN_DAY / configArr[SO_STORM_DURATION]) + 1;

    weatherStatusArr = (weatherStatusHandler*) malloc(sizeof(weatherStatusHandler) * weatherStatusLength);
    for (i = 0; i < weatherStatusLength; i++)
    {
        weatherStatusArr[i].pid = -1;
    }
    
    weatherId = configArr[SO_PORTI] + configArr[SO_NAVI] + 1;

    /* Acknowledge finish init */
    setAcknowledge();

    return 0;
}

int work() {

    int passedHours = 0;

    /* wait for simulation to start */
    raise(SIGSTOP);
    
    while (simulationRunning == 1)
    {   
        if (waitForSignal(SIGPOLL) != 0) {
            handleError("Error while waiting for hour");
            return -1;
        }

        if (checkWeatherStatus() != 0) {
            handleError("Error while checking weather status");
            return -1;
        }

        /* Activate storm and swell */
        if (passedHours % HOUR_IN_DAY == 0) {
            /*
            if (activateSwell() == -1) {
                handleError("Failed activating swall");
                return -1;
            }
            if (activateStorm() == -1) {
                handleError("Failed activating storm");
                return -1;
            }
*/
/* TODO Controllare perchè quando le barche sono in viaggio riesco a fare l'acknowledge, mentre se sono in una tempesta no*/
        }

        if (passedHours % configArr[SO_MALESTORM] == 0 && passedHours != 0) {
            
            if (activateMalestorm() == -1) {
                handleError("Failed activating malestorm");
                return -1;
            }
        }

        passedHours++;
    }

    return 0;
}

int activateSwell() {
    
    int randomPortId = getRandomValue(0, configArr[SO_PORTI] - 1);
    if (kill(portArr[randomPortId].pid, SIGPROF) == -1) {
        handleErrno("kill()");
        return -1;
    }
    
    if (insertNewWeatherStatus(portArr[randomPortId].pid, configArr[SO_SWELL_DURATION], SIGPROF) == -1) {
        handleError("Failed to insert new weather status");
        return -1;
    }

    return 0;
}

int activateStorm() {

    int randomBoatId = -1, i, randomStartId;
    randomStartId = getRandomValue(0, configArr[SO_NAVI] - 1);

    for (i = 0; i < configArr[SO_NAVI]; i++)
    {
        int currentId = (randomStartId + i) % configArr[SO_NAVI];
 
        if (boatArr[currentId].state == In_Sea_Travelling || boatArr[currentId].state == In_Sea_Empty_Travelling) {
            randomBoatId = currentId;
            break;
        }
    }

    if (randomBoatId != -1) {

        if (kill(boatArr[randomBoatId].pid, SIGPROF) == -1) {
            handleErrno("kill()");
            return -1;
        }
        
        if (insertNewWeatherStatus(boatArr[randomBoatId].pid, configArr[SO_STORM_DURATION], SIGPROF) == -1) {
            handleError("Failed to insert new weather status");
            return -1;
        }
    }

    return 0;
}

int activateMalestorm() {

    int randomBoatId = -1, i, randomStartId;
    randomStartId = getRandomValue(0, configArr[SO_NAVI] - 1);
    
    for (i = 0; i < configArr[SO_NAVI]; i++)
    {
        int currentId = (randomStartId + i) % configArr[SO_NAVI];
        if (boatArr[currentId].state != Sunk) {
            randomBoatId = currentId;
            break;
        }
    }

    if (randomBoatId != -1) {
        
        if (kill(boatArr[randomBoatId].pid, SIGPWR) == -1) {
            handleErrno("kill()");
            return -1;
        }
    }

    return 0;
}

int insertNewWeatherStatus(int pid, int remainingHours, int signalId) {

    int i;
    int foundSpace = 0;

    for (i = 0; i < weatherStatusLength; i++)
    {
        if (weatherStatusArr[i].pid == -1) {
            foundSpace = 1;
            weatherStatusArr[i].pid = pid;
            weatherStatusArr[i].remaningHour = remainingHours;
            weatherStatusArr[i].signalToSend = signalId;
            break;
        }
    }

    return foundSpace == 1 ? 0 : -1; 
}

int checkWeatherStatus() {

    int i;

    for (i = 0; i < weatherStatusLength; i++)
    {
        if (weatherStatusArr[i].pid != -1) {
            weatherStatusArr[i].remaningHour--;
            if (weatherStatusArr[i].remaningHour == 0) {
                if (kill(weatherStatusArr[i].pid, weatherStatusArr[i].signalToSend) == -1){
                    handleErrno("kill()");
                    return -1;
                }
                weatherStatusArr[i].pid = -1;
            }
        }
    }

    return 0;
}

void setAcknowledge() {

    acknowledgeInitArr[weatherId] = 1;
}

int cleanup() {

    free(weatherStatusArr);

    if (configArr != 0 && shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (boatArr != 0 && shmdt(boatArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (portArr != 0 && shmdt(portArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (acknowledgeInitArr != 0 && shmdt(acknowledgeInitArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    debug("Weather clean");

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    if (simulationRunning == 1 && masterPid == getppid()) {
        kill(getppid(), SIGINT);
    }
    exit(exitNumber);
}