#define _GNU_SOURCES

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <sys/msg.h>

#include <linux/limits.h>

#include "lib/analyzer.h"
#include "lib/msgPortProtocol.h"
#include "lib/utilities.h"

int *configArr;

int goodAnalyzerSharedMemoryId;
int boatAnalyzerSharedMemoryId; 
int portAnalyzerSharedMemoryId;

int readingMsgQueue;
int writingMsgQueue;

char *logPath;

int currentDay = 0;
int simulationRunning = 1;

void handle_analyzer_simulation_signals(int signal) {

    /* TODO Masks? */
    switch (signal)
    {
        case SIGUSR1:
        case SIGUSR2:
        case SIGCONT:
            break;

        /* End of the simulation */
        case SIGSYS:
            simulationRunning = 0;
            break;
        default:
            handleError("Intercept a unhandled signal");
            break;
    }
}

void handle_analyzer_stopProcess() {
    debug("Stopping analyzer...");
    cleanup();
    exit(0);
}

/* argv[0]=id | argv[1]=ganalizersh | argv[2]=banalyzersh | argv[3]=panalyzersh | argv[4]=wmsgq | argv[5]=rmsgq */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeSingalsHandlers();

    if (initializeConfig(argv[0]) == -1) {
        handleError("Initialization of analyzer config failed");
        safeExit(1);
    }

    if (initializeAnalyzer(argv[1], argv[2], argv[3], argv[4], argv[5]) == -1) {
        handleError("Initialization of analyzer failed");
        safeExit(2);
    }

    if (work() == -1) {
        handleError("Error during analyzer work");
        safeExit(3);
    }

    if (cleanup() == -1) {
        handleError("Analyzer cleanup failed");
        safeExit(4);
    }

    return 0;
}

int initializeSingalsHandlers() {

    struct sigaction signalAction;

    setpgid(getpid(), getppid());

    signal(SIGUSR1, handle_analyzer_simulation_signals);

    /* Use different method because i need to use the handler multiple times */
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_analyzer_simulation_signals;
    sigaction(SIGUSR2, &signalAction, NULL);
    
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_analyzer_simulation_signals;
    sigaction(SIGCONT, &signalAction, NULL);

    signal(SIGSYS, handle_analyzer_simulation_signals);
    signal(SIGINT, handle_analyzer_stopProcess);

    return 0;
}

int initializeConfig(char *configShareMemoryIdString) {

    char *p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        handleError("The config key as failed to be conveted in analyzer");
        return -1;
    }

    return 0;
}

int initializeAnalyzer(char *goodAnalyzerShareMemoryIdString, char *boatAnalyzerShareMemoryIdString, 
    char *portAnalyzerShareMemoryIdString, char *wmsgq, char *rmsgq) {

    char *p;

    goodAnalyzerSharedMemoryId = strtol(goodAnalyzerShareMemoryIdString, &p, 10);
    boatAnalyzerSharedMemoryId = strtol(boatAnalyzerShareMemoryIdString, &p, 10);
    portAnalyzerSharedMemoryId = strtol(portAnalyzerShareMemoryIdString, &p, 10);

    writingMsgQueue = strtol(wmsgq, &p, 10);
    readingMsgQueue = strtol(rmsgq, &p, 10);

    createLogFile();

    return 0;
}

int createLogFile() {
    char currentWorkingDirectory[PATH_MAX];
    FILE *filePointer;

    if (getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)) == NULL) {
        handleErrno("getcwd()");
        return -1;
    }

    logPath = malloc(PATH_MAX);
    sprintf(logPath, "%s/out/dump.txt", currentWorkingDirectory);

    filePointer = fopen(logPath, "w");
    if (filePointer == NULL) {
        if (errno == ENOENT) {

            /* File does not exist, create it */
            filePointer = fopen(logPath, "w");
            if (filePointer == NULL) {
                handleErrno("fopen() (create)");
                return -1;
            }
        } else {
            handleErrno("fopen()");
            return -1;
        }
    } else { 
        
        /* File exist, erase old data */
        if (ftruncate(fileno(filePointer), 0) != 0) {
            handleErrno("fileno()");
            return -1;
        }    
    }

    if (fclose(filePointer) != 0) {
        handleErrno("fclose()");
        return -1;
    }

    return 0;
}

int waitForNewDay() {

    PortMessage response;

    int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
    if (msgResponse == -1) {
        handleError("Error during waiting response from PA_NEW_DAY");
        return -1;
    }

    if (response.msg.data.action != PA_NEW_DAY) {
        handleError("Wrong action response instead of PA_NEW_DAY");
        return -1;
    }

    return 0;
}

int waitForStart() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGUSR1);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int work() {

    FILE *filePointer = fopen(logPath, "a");

    /* wait for simulation to start */
    if (waitForStart() != 0) {
        handleError("Error while waiting for start");
        return -1;
    }

    while (simulationRunning == 1)
    {
        if (waitForNewDay() != 0) {
            handleError("Error while waitnig for the new day by analyzer");
            return -1;
        }

        if (checkDataDump() == -1) {
            handleError("Error while check data dump");
            return -1;
        }

        if (filePointer == NULL) {
            handleError("Error opening file");
            return -1;
        }

        if (generateDailyHeader(filePointer) == -1) {
            handleError("Error while writing header");
            return -1;
        }

        if (generateDailyGoodReport(filePointer) == -1) {
            handleError("Error while writing good report");
            return -1;
        }

        if (generateDailyBoatReport(filePointer) == -1) {
            handleError("Error while writing boat report");
            return -1;
        }

        if (generateDailyPortReport(filePointer) == -1) {
            handleError("Error while writing port report");
            return -1;
        }
        
        currentDay++;

        if (sendMessage(writingMsgQueue, PA_FINISH, -1, -1) == -1) {
            handleError("Error during sendig of the PA_FINISH");
            return -1;
        }
    }

    if (fclose(filePointer) != 0) {
        handleErrno("flcose()");
        return -1;
    }

    return 0;
}

int checkDataDump() {

    int goodArrayLength = (configArr[SO_NAVI] + configArr[SO_PORTI]) * configArr[SO_MERCI];
    int i, goodSize, allDataCollected;
    double waitTimeInSeconds;
    goodDailyDump *goodArr; 
    boatDailyDump *boatArr; 
    portDailyDump *portArr;

    goodArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    boatArr = (boatDailyDump*) shmat(boatAnalyzerSharedMemoryId, NULL, 0);
    if (boatArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    portArr = (portDailyDump*) shmat(portAnalyzerSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    goodSize = configArr[SO_MERCI];
    waitTimeInSeconds = 0.005;
    do
    {
        long waitTimeInNs = getNanoSeconds(waitTimeInSeconds);
        if (safeWait(0, waitTimeInNs) == -1) {
            handleError("Error while waiting dump data in analyzer");
            return -1;
        }

        allDataCollected = 1;

        /* Check goods data */
        for (i = 0; i < goodArrayLength; i++)
        {
            /* Check if all goodId != 0 are set and not equal to 0 */
            if (i % goodSize != 0 && goodArr[i].goodId == 0) {
                debug("Failed good check");
                allDataCollected = 0;
                break;
            }
        }

        /* Check boat data */
        if (allDataCollected == 1) {
            for (i = 0; i < configArr[SO_NAVI]; i++)
            {
                if (boatArr[i].id == 0) {
                    debug("Failed boat check");
                    allDataCollected = 0;
                    break;
                }
            }
        }
        
        /* Check port data, we start from 1 to avoid port with id 0 */
        if (allDataCollected == 1) {
            for (i = 1; i < configArr[SO_PORTI]; i++)
            {
                if (portArr[i].id == 0) {
                    debug("Failed port check");
                    allDataCollected = 0;
                    break;
                }
            }
        }

        waitTimeInSeconds += waitTimeInSeconds;

        if (waitTimeInSeconds > 1) {
            handleError("Wait time for dump data exeaded 1 second");
            return -1;
        }

    } while (allDataCollected != 1);
    
    if (shmdt(goodArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(boatArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(portArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (sendMessage(writingMsgQueue, PA_DATA_COL, -1, -1) == -1) {
        handleError("Error during sendig of the PA_DATA_COL");
        return -1;
    }

    return 0;
}

int generateDailyHeader(FILE *filePointer) {

    fprintf(filePointer, "\nDay: %d\n", currentDay);

    return 0;
}

int generateDailyGoodReport(FILE *filePointer) {

    goodDailyDump *goodArr;
    int i, arraySize, goodArrayLength;    
    int **goodStatus;

    arraySize = configArr[SO_MERCI];
    goodStatus = malloc(sizeof(int *) * arraySize);
    for (i = 0; i < arraySize; i++) {
        /* 5 are the possible status of the good */
        goodStatus[i] = (int *) malloc(sizeof(int) * 5);
        memset(goodStatus[i], 0, sizeof(int) * 5);
    }

    goodArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    /* Aggregate data */
    goodArrayLength = (configArr[SO_NAVI] + configArr[SO_PORTI]) * configArr[SO_MERCI];
    for (i = 0; i < goodArrayLength; i++) {
        int goodId = goodArr[i].goodId;

        goodStatus[goodId][0] += goodArr[i].Good_In_The_Port;
        goodStatus[goodId][1] += goodArr[i].Good_In_The_Boat;
        goodStatus[goodId][2] += goodArr[i].Good_Delivered;
        goodStatus[goodId][3] += goodArr[i].Good_Expired_In_The_Port;
        goodStatus[goodId][4] += goodArr[i].Good_Expired_In_The_Boat;
    }

    /* Print data */
    fprintf(filePointer, "%-12s%-12s%-12s%-12s%-12s%-12s\n", "GOOD_ID", "G_PORT", "G_BOAT", "G_DELIV", "G_E_PORT", "G_E_BOAT");
    for (i = 0; i < arraySize; i++) {
        fprintf(filePointer, "%-12d%-12d%-12d%-12d%-12d%-12d\n", i, goodStatus[i][0], goodStatus[i][1], goodStatus[i][2],
            goodStatus[i][3], goodStatus[i][4]);
    }

    /* Cleaning of the memory after analyzing data */
    memset(goodArr, 0, goodArrayLength);

    if (shmdt(goodArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    free(goodStatus);

    return 0;
}

int generateDailyBoatReport(FILE *filePointer) {
    
    int *boatArr;

    /* After extraction */

    /* Cleaning of the memory after analyzing data */
    boatArr = (boatDailyDump*) shmat(boatAnalyzerSharedMemoryId, NULL, 0);
    if (boatArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    memset(boatArr, 0, configArr[SO_NAVI]);

    if (shmdt(boatArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int generateDailyPortReport(FILE *filePointer) {

    int *portArr;

    /* After extraction */

    /* Cleaning of the memory after analyzing data */
    portArr = (portDailyDump*) shmat(portAnalyzerSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    memset(portArr, 0, configArr[SO_PORTI]);

    if (shmdt(portArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int cleanup() { 

    free(logPath);

    if (shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmctl(goodAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }
    
    if (shmctl(boatAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    if (shmctl(portAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    if (msgctl(writingMsgQueue, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    if (msgctl(readingMsgQueue, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    debug("Analyzer clean");

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    kill(getppid(), SIGINT);
    exit(exitNumber);
}