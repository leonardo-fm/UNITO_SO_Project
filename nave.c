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
#include "lib/msgPortProtocol.h"

#include "lib/nave.h"

const int HOUR_IN_DAY = 24;

int *configArr = 0;

goodDailyDump *goodDumpArr = 0; 
boatDailyDump *boatDumpArr = 0; 
int *acknowledgeInitArr = 0;
int *acknowledgeDumpArr = 0;
goodEndDump *endGoodDumpArr = 0; 
sem_t *endGoodDumpSemaphore = 0;
Port *portArr = 0; 
Boat *boatArr = 0; 

int currentMsgQueueId = -1;
int currentPort = -1;
int readingMsgQueue = -1;
int writingMsgQueue = -1;

/* No value equal to -1, o to n is the good id is selling */
int currentSellingGood = -1;

Boat boat;
Goods *goodHold;

int simulationRunning = 1;
int masterPid = 0;

void handle_boat_simulation_signals(int signal) {
/* TODO controllare maskere */
    switch (signal)
    {
        case SIGUSR1: /* Start simulation */
        case SIGIO: /* Dump data */
        case SIGCONT: /* Continue fimulation */
            break;

        case SIGUSR2: /* Stop simulation */
            setAcknowledge();

            waitForSignal(SIGIO);
            setAcknowledge();
            
            dumpData();
            waitForSignal(SIGCONT);
            setAcknowledge();
            
            newDay();
            break;

        case SIGPROF: /* Storm */
            handleStorm();
            break;

        case SIGPWR: /* Malestorm */
            handleMalestorm();
            simulationRunning = 0;
            break;

        case SIGSYS: /* End simulation */
            dumpData();
            simulationRunning = 0;
            break;
            
        default:
            handleError("Intercept a unhandled signal");
            break;
    }
}

void handle_boat_stopProcess() {

    /* Block all incoming signals after the first SIGINT */
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    debug("Stopping boat...");
    cleanup();
    exit(0);
}

/* argv[0]=id | argv[1]=configsh | argv[2]=portsh | argv[3]=ganalizersh | 
    argv[4]=banalyzersh | argv[5]=akish | argv[6]=endanalyzershm | argv[7]=akdsh | argv[8]=bsh */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeEnvironment();
    initializeSingalsMask();
    initializeSingalsHandlers();

    if (initializeConfig(argv[1], argv[3], argv[4], argv[5], argv[6], argv[7]) == -1) {
        handleError("Initialization of boat config failed");
        safeExit(1);
    }
    
    if (initializeBoat(argv[0], argv[2], argv[8]) == -1) {
        handleError("Initialization of boat failed");
        safeExit(2);
    }

    if (work() == -1) {
        handleError("Error during boat work");
        safeExit(3);
    }

    /* Aknowledge finish */
    setAcknowledge();

    if (cleanup() == -1) {
        handleError("Boat cleanup failed");
        safeExit(4);
    }

    return 0;
}

void initializeSingalsMask() {

    sigset_t sigMask;

    sigfillset(&sigMask);
    sigdelset(&sigMask, SIGUSR1);
    sigdelset(&sigMask, SIGUSR2);
    sigdelset(&sigMask, SIGIO);
    sigdelset(&sigMask, SIGCONT);
    sigdelset(&sigMask, SIGPROF);
    sigdelset(&sigMask, SIGPWR);
    sigdelset(&sigMask, SIGSYS);
    sigdelset(&sigMask, SIGINT);
    sigprocmask(SIG_SETMASK, &sigMask, NULL);
}

int initializeSingalsHandlers() {

    struct sigaction signalAction;

    setpgid(getpid(), getppid());
    masterPid = getppid();

    signal(SIGUSR1, handle_boat_simulation_signals);

    /* Use different method because i need to use the handler multiple times */
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_boat_simulation_signals;
    sigaction(SIGUSR2, &signalAction, NULL);

    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_boat_simulation_signals;
    sigaction(SIGIO, &signalAction, NULL);

    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_boat_simulation_signals;
    sigaction(SIGCONT, &signalAction, NULL);

    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_boat_simulation_signals;
    sigaction(SIGPROF, &signalAction, NULL);

    signal(SIGPWR, handle_boat_simulation_signals);
    signal(SIGSYS, handle_boat_simulation_signals);
    signal(SIGINT, handle_boat_stopProcess);

    return 0;
}

int initializeConfig(char *configSharedMemoryIdString, char *goodAnalyzerSharedMemoryIdString, 
    char *boatAnalyzerSharedMemoryIdString, char *acknowledgeInitSharedMemoryIdS, char *endGoodSharedMemoryIdS,
    char *acknowledgeDumpSharedMemoryIdS) {

    char *p;

    int configSharedMemoryId = strtol(configSharedMemoryIdString, &p, 10);
    int goodAnalyzerSharedMemoryId = strtol(goodAnalyzerSharedMemoryIdString, &p, 10);
    int boatAnalyzerSharedMemoryId = strtol(boatAnalyzerSharedMemoryIdString, &p, 10);
    int acknowledgeInitSharedMemoryId = strtol(acknowledgeInitSharedMemoryIdS, &p, 10);
    int endGoodSharedMemoryId = strtol(endGoodSharedMemoryIdS, &p, 10);
    int acknowledgeDumpSharedMemoryId = strtol(acknowledgeDumpSharedMemoryIdS, &p, 10);
    
    configArr = (int*) shmat(configSharedMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    goodDumpArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    boatDumpArr = (boatDailyDump*) shmat(boatAnalyzerSharedMemoryId, NULL, 0);
    if (boatDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    acknowledgeInitArr = (int*) shmat(acknowledgeInitSharedMemoryId, NULL, 0);
    if (acknowledgeInitArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    endGoodDumpArr = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (endGoodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    endGoodDumpSemaphore = sem_open(endGoodSharedMemoryIdS, O_EXCL, 0600, 1);
    if (endGoodDumpSemaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    acknowledgeDumpArr = (int*) shmat(acknowledgeDumpSharedMemoryId, NULL, 0);
    if (acknowledgeDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    return 0;
}

int initializeBoat(char *boatIdS, char *portSharedMemoryIdS, char *boatSharedMemoryIdS) {

    int referenceId, i = 0;
    char *p;
    int portSharedMemoryId = strtol(portSharedMemoryIdS, &p, 10);
    int boatSharedMemoryId = strtol(boatSharedMemoryIdS, &p, 10);

    portArr = (Port*) shmat(portSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    boatArr = (Boat*) shmat(boatSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    boat.id = strtol(boatIdS, &p, 10);
    boat.pid = getpid();
    boat.position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    boat.capacityInTon = configArr[S0_CAPACITY];
    boat.speed = configArr[SO_SPEED];
    boat.state = In_Sea_Empty;

    /* Added boat to the array of boats */
    referenceId = boat.id - configArr[SO_PORTI];
    boatArr[referenceId] = boat;

    /* Initialization of the hold */
    goodHold = malloc(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodHold == NULL) {
        handleError("Error during malloc of boat hold");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods emptyGood;
        emptyGood.id = i;
        emptyGood.loadInTon = 0;
        emptyGood.state = Undefined;
        emptyGood.remaningDays = 0;

        goodHold[i] = emptyGood;
    }
    
    setAcknowledge();

    return 0;
}

int handleStorm() {

    double stormTime = getNanoSeconds((double) 1 / HOUR_IN_DAY) * configArr[SO_STORM_DURATION];
    double waitTimeNs = getNanoSeconds(stormTime);
    double waitTimeS = getSeconds(stormTime);

    boat.storm++;
    if (safeWait(waitTimeS, waitTimeNs) == -1) {
        handleError("Error while waiting the storm");
        return -1;
    }

    return 0;
}

void handleMalestorm() {

    /* Block all incoming signals after the first SIGINT */
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    dumpData();

    boat.state = Sunk;
    acknowledgeInitArr[boat.id] = -1;
    acknowledgeDumpArr[boat.id] = -1;
}

int work() {

    /* wait for simulation to start */
    if (waitForSignal(SIGUSR1) != 0) {
        handleError("Error while waiting for start");
        return -1;
    }

    while (simulationRunning == 1)
    {   
        int tradeStatus;

        if (simulationRunning == 1) {
            if (gotoPort() == -1) {
                handleError("Error while going to a port");
                return -1;
            }
        }

        if (simulationRunning == 1) {
            if (setupTrade(currentPort) == -1) {
                handleError("Error during setup for the trade");
                return -1;
            }

            tradeStatus = openTrade();
            if (tradeStatus == -1) {
                handleError("Error during trade");
                return -1;
            } 

            if (haveIGoodsToSell() == 0) {
                boat.state = In_Sea;
            } else {
                boat.state = In_Sea_Empty;
            }
        }
    }

    return 0;
}

int dumpData() {

    int i, goodReferenceId, boatReferenceId;
    
    goodDailyDump gdd;
    boatDailyDump bdd;

    /* Send goods data */
    goodReferenceId = boat.id * configArr[SO_MERCI];
    for (i = 0; i < configArr[SO_MERCI]; i++) {

        gdd.goodId = i;
        if (goodHold[i].state == Expired_In_The_Boat) {
            gdd.Good_Expired_In_The_Boat = goodHold[i].loadInTon;
            
            goodHold[i].loadInTon = 0;
            goodHold[i].state = Undefined;
        } else {
            gdd.Good_Expired_In_The_Boat = 0;
        }
        gdd.Good_In_The_Boat = goodHold[i].loadInTon;

        gdd.Good_Delivered = 0;
        gdd.Good_Expired_In_The_Port = 0;
        gdd.Good_In_The_Port = 0;

        memcpy(&goodDumpArr[goodReferenceId + i], &gdd, sizeof(goodDailyDump)); 
    }

    /* Send boat data */
    boatReferenceId = boat.id - configArr[SO_PORTI];

    bdd.id = boat.id;
    bdd.storm = boat.storm;
    bdd.malestorm = boat.state == Sunk ? 1 : 0;
    bdd.boatState = boat.state;
    memcpy(&boatDumpArr[boatReferenceId], &bdd, sizeof(boatDailyDump)); 

    /* Acknowledge finish dump data */
    acknowledgeDumpArr[boat.id] = 1;

    return 0;
}

int newDay() {

    int i;

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(goodHold[i].remaningDays > 0) {
            goodHold[i].remaningDays--;
            /* Avoid adding sold expired good */
            if (goodHold[i].remaningDays == 0 && currentSellingGood != i) {
                goodHold[i].state = Expired_In_The_Boat;

                sem_wait(endGoodDumpSemaphore);
                endGoodDumpArr[i].expiredInBoat += goodHold[i].loadInTon;
                sem_post(endGoodDumpSemaphore);
            }
        }
    }

    return 0;
}

int gotoPort() {

    int newPortFound = 0;
    double distance, kmPerDay;
    long waitTimeNs;
    int waitTimeS;

    while (newPortFound == 0)
    {
        int randomPort = getRandomValue(0, configArr[SO_PORTI] - 1);

        if (randomPort != currentPort) {
            newPortFound = 1;
            currentPort = randomPort;
        }
    }

    distance = sqrt(pow(portArr[currentPort].position.x - boat.position.x, 2) 
                    + pow(portArr[currentPort].position.y - boat.position.y, 2));

    kmPerDay = distance / configArr[SO_SPEED];
    waitTimeNs = getNanoSeconds(kmPerDay);
    waitTimeS = getSeconds(kmPerDay);

    /* Set the status to travellig */
    boat.state = boat.state == In_Sea ? In_Sea_Travelling : In_Sea_Empty_Travelling;

    if (safeWait(waitTimeS, waitTimeNs) == -1) {
        handleError("Error while waiting to go to in a port");
        return -1;
    }

    boat.state = boat.state == In_Sea_Travelling ? In_Sea : In_Sea_Empty;

    return 0;
}

int setupTrade(int portId) {

    readingMsgQueue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    writingMsgQueue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);

    if (currentMsgQueueId != -1) {
        handleError("The old comunication was not closed properly");
        return -1;
    }

    currentMsgQueueId = portArr[portId].msgQueuId;
    if (sendMessage(currentMsgQueueId, PA_SETUP, writingMsgQueue, readingMsgQueue) == -1) {
        handleError("Failed to send SETUP comunication");
        return -1;
    }

    return 0;
}

/* Return 0 if the trade is a success, 1 if the port refuse to accept the boat, -1 for errors */
int openTrade() {

    int waitResponse;
    PortMessage response;

    if (sendMessage(writingMsgQueue, PA_ACCEPT, 0, 0) == -1) {
        handleError("Failed to send ACCEPT comunication");
        return -1;
    }

    /* Wait for the response */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            handleError("Error during waiting response from ACCEPT");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }
    
    if (response.msg.data.action == PA_N) {

        if (msgctl(readingMsgQueue, IPC_RMID, NULL) == -1) {
            handleErrno("msgctl()");
            return -1;
        }

        if (msgctl(writingMsgQueue, IPC_RMID, NULL) == -1) {
            handleErrno("msgctl()");
            return -1;
        }

        currentMsgQueueId = -1;

        return 1;
    }
    
    boat.state = In_Port_Exchange;
    
    if (trade() == -1) {
        handleError("Error during trade");
        return -1;
    }

    return 0;
}

int trade() {

    if (haveIGoodsToSell() == 0 && simulationRunning == 1) {

        if (sellGoods() == -1) {
            handleError("Error during selling goods");
            return -1;
        }
    }

    if (haveIGoodsToBuy() == 0 && simulationRunning == 1) {

        if (buyGoods() == -1) {
            handleError("Error during buying goods");
            return -1;
        }
    }   

    if (closeTrade() == -1) {
        handleError("Error during closing trade");
        return -1;
    }

    return 0;
}

int closeTrade() {

    int waitResponse;
    PortMessage response;

    if (sendMessage(writingMsgQueue, PA_EOT, 0, 0) == -1) {
        handleError("Failed to send EOT comunication");
        return -1;
    }

    /* Default response to avoid random numbers if not initialized  */
    response.msg.data.action = PA_EOT;
    
    /* Wait for the response of end of transmission */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            handleError("Error during waiting response from EOT");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }

    if (response.msg.data.action == PA_EOT) {

        currentMsgQueueId = -1;

        if (msgctl(readingMsgQueue, IPC_RMID, NULL) == -1) {
            handleErrno("msgctl()");
            return -1;
        }
        readingMsgQueue = -1;

        if (msgctl(writingMsgQueue, IPC_RMID, NULL) == -1) {
            handleErrno("msgctl()");
            return -1;
        }
        writingMsgQueue = -1;
    } else {
        handleError("The boat while waiting for the EOT from the port recived something else");
    }

    return 0;
}

/* Return 0 if there are goods in the boat available for sale, otherwise -1 */
int haveIGoodsToSell() {
    
    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodHold[i].loadInTon > 0 ) {
            return 0;
        }    
    }

    return -1;
}

/* Return 0 if there are space available in the hold, otherwise -1 */
int haveIGoodsToBuy() {
    
    int i = 0;
    int totalNumOfTons = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        totalNumOfTons += goodHold[i].loadInTon;   
    }

    if (totalNumOfTons < boat.capacityInTon) {
        return 0;
    } else {
        return -1;
    }
}

int sellGoods() {

    int waitResponse, i;
    PortMessage response;

    char semaphoreKey[12];
    sem_t *semaphore;

    Goods *goodArr;

    /* Send request to sell */
    if (simulationRunning == 1) {
        if (sendMessage(writingMsgQueue, PA_SE_GOOD, 0, 0) == -1) {
            handleError("Failed to send PA_SE_GOOD comunication");
            return -1;
        }
    }

    /* Default response to avoid random numbers if not initialized */
    response.msg.data.action = PA_N;

    /* Wait response */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            handleError("Error during waiting response from PA_SE_GOOD");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }

    if (response.msg.data.action == PA_N) {
        return 0;
    }

    /* Get semaphore */
    if (sprintf(semaphoreKey, "%d", response.msg.data.data2) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    /* Get shared memory of the port */
    goodArr = (Goods*) shmat(response.msg.data.data1, NULL, 0);
    if (goodArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    /* Sell all available goods */
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodHold[i].loadInTon > 0 && goodHold[i].state != Expired_In_The_Boat && boat.state == In_Port_Exchange) {

            int exchange = 0;
            double loadTonPerDay;
            long waitTimeS, waitTimeNs;

            if (goodArr[i].loadInTon == 0) {
                continue;
            }
            
            sem_wait(semaphore);
            currentSellingGood = i;

            /* If x >= 0 OK, x < 0 not enought good to sell */
            if (goodArr[i].loadInTon - goodHold[i].loadInTon >= 0) {
                exchange = goodHold[i].loadInTon;
                goodArr[i].loadInTon -= goodHold[i].loadInTon;
                goodHold[i].loadInTon = 0;
                goodHold[i].remaningDays = 0;
            } else {
                exchange = goodHold[i].loadInTon - goodArr[i].loadInTon;
                goodArr[i].loadInTon = 0;
                goodHold[i].loadInTon -= exchange;
            }

            /* Set good state for boat */
            if (goodHold[i].loadInTon == 0) {
                goodHold[i].state = Undefined;
            }

            /* Set good state for port */
            if (goodArr[i].loadInTon > 0) {
                goodArr[i].state = In_The_Port;
            }

            currentSellingGood = -1;
            sem_post(semaphore);

            loadTonPerDay = (double) exchange / configArr[SO_LOADSPEED];
            waitTimeNs = getNanoSeconds(loadTonPerDay);
            waitTimeS = getSeconds(loadTonPerDay);

            if (safeWait(waitTimeS, waitTimeNs) == -1) {
                handleError("Error while waiting to exchange");
                return -1;
            }

            /* Send sell report to the port */
            if (sendMessage(writingMsgQueue, PA_SE_SUMMARY, i, exchange) == -1) {
                handleError("Failed to send PA_SE_SUMMARY comunication");
                return -1;
            }
        }    
    }

    if (sem_close(semaphore) == -1) {
        handleErrno("sem_close()");
        return -1;
    }

    if (shmdt(goodArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int buyGoods() {

    int waitResponse, i, availableSpace;
    PortMessage response;
    
    char semaphoreKey[12];
    sem_t *semaphore;

    Goods *goodArr;
    
    /* Send request to buy */
    if (simulationRunning == 1) {
        if (sendMessage(writingMsgQueue, PA_RQ_GOOD, 0, 0) == -1) {
            handleError("Failed to send PA_RQ_GOOD comunication");
            return -1;
        }
    }

    /* Default response to avoid random numbers if not initialized */
    response.msg.data.action = PA_N;

    /* Wait response */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            handleError("Error during waiting response from PA_RQ_GOOD");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }

    if (response.msg.data.action == PA_N) {
        return 0;
    }

    /* Get semaphore */
    if (sprintf(semaphoreKey, "%d", response.msg.data.data2) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    /* Get shared memory of the port */
    goodArr = (Goods*) shmat(response.msg.data.data1, NULL, 0);
    if (goodArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    /* Buy some available goods */
    availableSpace = floor((double) getSpaceAvailableInTheHold() / configArr[SO_MERCI]);
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodArr[i].loadInTon > 0 && goodArr[i].state != Expired_In_The_Port && boat.state == In_Port_Exchange) {
            
            int exchange;
            double loadTonPerDay;
            long waitTimeS, waitTimeNs;

            sem_wait(semaphore);

            exchange = 0;

            /* If x >= 0 OK, x < 0 not enought good to buy */
            if (goodArr[i].loadInTon - availableSpace >= 0) {
                exchange = availableSpace;
                goodArr[i].loadInTon -= availableSpace;
                goodHold[i].loadInTon += availableSpace;
            } else {
                exchange = goodArr[i].loadInTon;
                goodArr[i].loadInTon = 0;
                goodHold[i].loadInTon += exchange;
            }

            /* Set expire date */
            goodHold[i].remaningDays = goodArr[i].remaningDays;

            /* Set good state for boat */
            if (goodHold[i].loadInTon > 0) {
                goodHold[i].state = In_The_Boat;
            }

            /* Set good state for port */
            if (goodArr[i].loadInTon == 0) {
                goodArr[i].state = Undefined;
            }

            sem_post(semaphore);

            loadTonPerDay = (double) exchange / configArr[SO_LOADSPEED];
            waitTimeNs = getNanoSeconds(loadTonPerDay);
            waitTimeS = getSeconds(loadTonPerDay);

            if (safeWait(waitTimeS, waitTimeNs) == -1) {
                handleError("Error while waiting to exchange");
                return -1;
            }
            
            /* Send request report to the port */
            if (sendMessage(writingMsgQueue, PA_RQ_SUMMARY, i, exchange) == -1) {
                handleError("Failed to send PA_RQ_SUMMARY comunication");
                return -1;
            }

            if (getSpaceAvailableInTheHold() == 0) {
                break;
            }
        }    
    }

    if (sem_close(semaphore) == -1) {
        handleErrno("sem_close()");
        return -1;
    }

    if (shmdt(goodArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }
    
    return 0;
}

/* Return how many space have in the boat hold */
int getSpaceAvailableInTheHold() {

    int i = 0;
    int totalNumOfTons = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        totalNumOfTons += goodHold[i].loadInTon;   
    }

    return boat.capacityInTon - totalNumOfTons;
}

void setAcknowledge() {

    if (boat.state != Sunk) {
        acknowledgeInitArr[boat.id] = 1;
    }
}

int cleanup() {

    if (simulationRunning == 1) {
        msgctl(readingMsgQueue, IPC_RMID, NULL);
    }

    if (simulationRunning == 1) {
        msgctl(writingMsgQueue, IPC_RMID, NULL);
    }

    if (goodDumpArr != 0 && shmdt(goodDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (boatDumpArr != 0 && shmdt(boatDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (acknowledgeInitArr != 0 && shmdt(acknowledgeInitArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (acknowledgeDumpArr != 0 && shmdt(acknowledgeDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (endGoodDumpSemaphore != 0 && sem_close(endGoodDumpSemaphore) == -1) {
        handleErrno("sem_close()");
        return -1;
    }

    if (portArr != 0 && shmdt(portArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (configArr != 0 && shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    debug("Boat clean");

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    if (simulationRunning == 1 && masterPid == getppid()) {
        kill(getppid(), SIGINT);
    }
    exit(exitNumber);
}