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
goodDailyDump *goodDumpArr;

int boatAnalyzerSharedMemoryId; 
boatDailyDump *boatDumpArr;

int portAnalyzerSharedMemoryId;
portDailyDump *portDumpArr;

int acknowledgeShareMemoryId;
int *acknowledgeArr;

int endGoodSharedMemoryId; 
goodEndDump *endGoodArr;

/* Malloc */
int *endPortArr; 

int readingMsgQueue;
int writingMsgQueue;

char *logPath;
FILE *filePointer;

int currentDay = 0;
int simulationRunning = 1;
int simulationFinishedEarly = 0;

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

/* argv[0]=id | argv[1]=ganalizersh | argv[2]=banalyzersh | argv[3]=panalyzersh | 
    argv[4]=wmsgq | argv[5]=rmsgq | argv[6]=endanalyzershm, argv[7]=akish */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeSingalsHandlers();

    if (initializeConfig(argv[0]) == -1) {
        handleError("Initialization of analyzer config failed");
        safeExit(1);
    }

    if (initializeAnalyzer(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]) == -1) {
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
    char *portAnalyzerShareMemoryIdString, char *wmsgq, char *rmsgq, char *endGoodShareMemoryIdString,
    char *acknowledgeShareMemoryIdString) {

    char *p;
    int size = sizeof(int) * configArr[SO_PORTI] * 2;

    goodAnalyzerSharedMemoryId = strtol(goodAnalyzerShareMemoryIdString, &p, 10);
    goodDumpArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    boatAnalyzerSharedMemoryId = strtol(boatAnalyzerShareMemoryIdString, &p, 10);
    boatDumpArr = (boatDailyDump*) shmat(boatAnalyzerSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    portAnalyzerSharedMemoryId = strtol(portAnalyzerShareMemoryIdString, &p, 10);
    portDumpArr = (portDailyDump*) shmat(portAnalyzerSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    acknowledgeShareMemoryId = strtol(acknowledgeShareMemoryIdString, &p, 10);
    acknowledgeArr = (int*) shmat(acknowledgeShareMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    endGoodSharedMemoryId = strtol(endGoodShareMemoryIdString, &p, 10);
    endGoodArr = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    writingMsgQueue = strtol(wmsgq, &p, 10);
    readingMsgQueue = strtol(rmsgq, &p, 10);

    endPortArr = (int *) malloc(size);
    memset(endPortArr, 0, size);

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

    filePointer = fopen(logPath, "a");

    /* wait for simulation to start */
    if (waitForStart() != 0) {
        handleError("Error while waiting for start");
        return -1;
    }

    if (filePointer == NULL) {
        handleError("Error opening file");
        return -1;
    }

    if (waitForNewDay() != 0) {
        handleError("Error while waitnig for the new day by analyzer");
        return -1;
    }

    while (simulationRunning == 1)
    {
        if (checkDataDump() == -1) {
            handleError("Error while check data dump");
            return -1;
        }

        /* Check all entetyes has write in the dump shm befoure closing all */
        if (simulationFinishedEarly == 1) {
            break;
        }

        if (generateDailyDump() == -1) {
            handleError("Error while daily dump");
            return -1;
        }
        
        currentDay++;

        if (sendMessage(writingMsgQueue, PA_FINISH, -1, -1) == -1) {
            handleError("Error during sendig of the PA_FINISH");
            return -1;
        }

        if (waitForNewDay() != 0) {
            handleError("Error while waitnig for the new day by analyzer");
            return -1;
        }
    }
    
    if (generateEndDump() == -1) {
        handleError("Error while end dump");
        return -1;
    }

    if (fclose(filePointer) != 0) {
        handleErrno("flcose()");
        return -1;
    }

    return 0;
}

int checkDataDump() {

    int i, allDataCollected; 
    int entities = configArr[SO_NAVI] + configArr[SO_PORTI];
    double waitTimeInSeconds;
    char buffer[32];

    waitTimeInSeconds = 0.005;
    do
    {
        long waitTimeInNs = getNanoSeconds(waitTimeInSeconds);
        if (safeWait(0, waitTimeInNs) == -1) {
            handleError("Error while waiting dump data in analyzer");
            return -1;
        }

        allDataCollected = 1;

        /* Check data */
        for (i = 0; i < entities; i++)
        {
            /* Check if all goodId != 0 are set and not equal to 0 */
            if (acknowledgeArr[i] == 0) {
                snprintf(buffer, sizeof(buffer), "Failed check on %d", i);
                debug(buffer);
                allDataCollected = 0;
                break;
            }
        }

        waitTimeInSeconds += waitTimeInSeconds;

        if (waitTimeInSeconds > 1) {
            handleError("Wait time for dump data exeaded 1 second");
            return -1;
        }

    } while (allDataCollected != 1);

    if (sendMessage(writingMsgQueue, PA_DATA_COL, -1, -1) == -1) {
        handleError("Error during sendig of the PA_DATA_COL");
        return -1;
    }

    /* Reset acknowledge */
    memset(acknowledgeArr, 0, entities);

    return 0;
}

int generateDailyDump() {

    if (generateDailyHeader() == -1) {
        handleError("Error while writing header");
        return -1;
    }

    if (generateDailyGoodReport() == -1) {
        handleError("Error while writing good report");
        return -1;
    }

    if (generateDailyBoatReport() == -1) {
        handleError("Error while writing boat report");
        return -1;
    }

    if (generateDailyPortReport() == -1) {
        handleError("Error while writing port report");
        return -1;
    }

    return 0;
}

int generateDailyHeader() {

    fprintf(filePointer, "Day: %d\n\n", currentDay);

    return 0;
}

int generateDailyGoodReport() {

    int i, arraySize, goodArrayLength;    
    int **goodStatus;

    arraySize = configArr[SO_MERCI];
    goodStatus = malloc(sizeof(int *) * arraySize);
    for (i = 0; i < arraySize; i++) {
        /* 5 are the possible status of the good */
        goodStatus[i] = (int *) malloc(sizeof(int) * 5);
        memset(goodStatus[i], 0, sizeof(int) * 5);
    }

    /* Aggregate data */
    goodArrayLength = (configArr[SO_NAVI] + configArr[SO_PORTI]) * configArr[SO_MERCI];
    for (i = 0; i < goodArrayLength; i++) {
        int goodId = goodDumpArr[i].goodId;

        goodStatus[goodId][0] += goodDumpArr[i].Good_In_The_Port;
        goodStatus[goodId][1] += goodDumpArr[i].Good_In_The_Boat;
        goodStatus[goodId][2] += goodDumpArr[i].Good_Delivered;
        goodStatus[goodId][3] += goodDumpArr[i].Good_Expired_In_The_Port;
        goodStatus[goodId][4] += goodDumpArr[i].Good_Expired_In_The_Boat;
    }

    /* Print data */
    fprintf(filePointer, "%-12s%-12s%-12s%-12s%-12s%-12s\n", "GOOD_ID", "G_PORT", "G_BOAT", "G_DELIV", "G_E_PORT", "G_E_BOAT");
    for (i = 0; i < arraySize; i++) {
        fprintf(filePointer, "%-12d%-12d%-12d%-12d%-12d%-12d\n", i, goodStatus[i][0], goodStatus[i][1], goodStatus[i][2],
            goodStatus[i][3], goodStatus[i][4]);
    }
    fprintf(filePointer, "\n");

    /* Cleaning of the memory after analyzing data */
    memset(goodDumpArr, 0, goodArrayLength);

    free(goodStatus);

    return 0;
}

int generateDailyBoatReport() {
    
    /* In_Sea = 0, In_Sea_Empty = 1, In_Port_Exchange = 2 */
    int boatStatus[3] = {0};
    int i;

    /* Aggregate data */
    for (i = 0; i < configArr[SO_NAVI]; i++) {
        boatStatus[boatDumpArr[i].boatState]++;
    }

    /* Print data */
    fprintf(filePointer, "%-12s%-12s%-12s\n", "BOAT_SEA", "BOAT_SEA_E", "BOAT_EXCH");
    fprintf(filePointer, "%-12d%-12d%-12d\n", boatStatus[0], boatStatus[1], boatStatus[2]);
    fprintf(filePointer, "\n");

    /* Cleaning of the memory after analyzing data */
    memset(boatDumpArr, 0, configArr[SO_NAVI]);

    return 0;
}

int generateDailyPortReport() {

    int i, inStock, requested;

    /* Print data */
    fprintf(filePointer, "%-12s%-12s%-12s%-12s%-12s%-12s\n", "PORT_ID", "G00D_STOCK", "GOOD_REQ", "GOOD_SOLD", "GOOD_RECIV", "BUSY_QUAYS");
    for (i = 0; i < configArr[SO_PORTI]; i++) {
        char buffer[16];
        int currentId;

        snprintf(buffer, sizeof(buffer), "%d/%d", portDumpArr[i].busyQuays, portDumpArr[i].totalQuays);
        fprintf(filePointer, "%-12d%-12d%-12d%-12d%-12d%-12s\n", portDumpArr[i].id, portDumpArr[i].totalGoodInStock, 
            portDumpArr[i].totalGoodRequested, portDumpArr[i].totalGoodSold, portDumpArr[i].totalGoodRecived, buffer);

        /* Collect data for the best port at the end dump */
        currentId = 2 * i;
        endPortArr[currentId] += portDumpArr[i].totalGoodSold;
        endPortArr[currentId + 1] += portDumpArr[i].totalGoodRecived;
    }
    fprintf(filePointer, "\n");

    /* Check for possible ending of the simulation */
    inStock = 0;
    requested = 0;
    for (i = 0; i < configArr[SO_PORTI]; i++) {

        inStock += portDumpArr[i].totalGoodInStock;
        requested += portDumpArr[i].totalGoodRequested;
    }

    if (inStock == 0 || requested == 0) {
        sendMessage(writingMsgQueue, PA_EOS_GSR, -1, -1);
        simulationFinishedEarly = 1;
        debug("Analyzer sent PA_EOS_GSR to master");
    }

    /* Cleaning of the memory after analyzing data */
    memset(portDumpArr, 0, configArr[SO_PORTI]);

    return 0;
}

int generateEndDump() {

    if (generateEndHeader() == -1) {
        handleError("Error while writing header");
        return -1;
    }

    if (generateDailyGoodReport() == -1) {
        handleError("Error while writing good report");
        return -1;
    }

    if (generateDailyBoatReport() == -1) {
        handleError("Error while writing boat report");
        return -1;
    }

    if (generateDailyPortReport() == -1) {
        handleError("Error while writing port report");
        return -1;
    }

    if (generateEndGoodReport() == -1) {
        handleError("Error while writing end good report");
        return -1;
    }

    if (generateEndPortStat() == -1) {
        handleError("Error while writing end port report");
        return -1;
    }
    
    return 0;
}

int generateEndHeader() {

    fprintf(filePointer, "End of simulation\n\n");

    return 0;
}

int generateEndGoodReport() {

    int i;    

    /* Print data */
    fprintf(filePointer, "%-12s%-12s%-12s%-12s%-12s%-12s\n", "GOOD_ID", "INIT_TOTAL", "IN_PORT", "E_IN_PORT", "E_IN_BOAT", "EXCHANGED");
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        fprintf(filePointer, "%-12d%-12d%-12d%-12d%-12d%-12d\n", i, endGoodArr[i].totalInitNumber, endGoodArr[i].inPort,
            endGoodArr[i].expiredInPort, endGoodArr[i].expiredInBoat, endGoodArr[i].exchanged);
    }
    fprintf(filePointer, "\n");

    return 0;
}

int generateEndPortStat() {

    int i, portIdSold = 0, totSold = 0, portIdReq = 0, totReq = 0;

    for (i = 0; i < configArr[SO_PORTI]; i++) {
        int currentId = i * 2;

        if (endPortArr[currentId] > totSold) {
            totSold = endPortArr[currentId];
            portIdSold = i;
        }
        if (endPortArr[currentId + 1] > totReq) {
            totReq = endPortArr[currentId + 1];
            portIdReq = i;
        }
    }

    fprintf(filePointer, "The port %d have sold %d ton of goods\n", portIdSold, totSold);
    fprintf(filePointer, "The port %d have requested %d ton of goods\n", portIdReq, totReq);

    return 0;
}

int cleanup() { 

    free(logPath);
    free(endPortArr);

    if (shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(goodDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmctl(goodAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    if (shmdt(boatDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }
    
    if (shmctl(boatAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    if (shmdt(portDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmctl(portAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    if (shmdt(endGoodArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmctl(endGoodSharedMemoryId, IPC_RMID, NULL) == -1) {
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

    if (shmdt(acknowledgeArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmctl(acknowledgeShareMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    debug("Analyzer clean");

    return 0;
}

void safeExit(int exitNumber) {
    
    fclose(filePointer);
    cleanup();
    if (simulationRunning == 1) {
        kill(getppid(), SIGINT);
    }
    exit(exitNumber);
}