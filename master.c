#define _GNU_SOURCES

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>    
#include <sys/shm.h>    
#include <signal.h> 
#include <sys/msg.h>

#include "lib/config.h"
#include "lib/utilities.h"
#include "lib/msgPortProtocol.h"

#include "lib/master.h"

int configShareMemoryId;
int goodShareMemoryId;
int portShareMemoryId;

int goodAnalyzerShareMemoryId;
int acknowledgeInitShareMemoryId;
int boatAnalyzerShareMemoryId;
int portAnalyzerShareMemoryId;

int endGoodAnalyzerShareMemoryId;

int analyzerReadingMsgQueue;
int analyzerWritingMsgQueue;

int NUM_OF_SETTINGS = 13;
int *configArr;
int currentProcessId;
int simulationFinishedEarly = 0;

void handle_master_stopProcess() { 

    debug("\nStopping program...");
    killpg(getpid(), SIGINT);
    cleanup();
    exit(0);
}

int main() {

    int analyzerArgs[8];
    int portArgs[7];
    int boatArgs[6];

    setpgid(getpid(), getpid());

    /* ----- CONFIG ----- */
    configShareMemoryId = generateShareMemory(sizeof(int) * NUM_OF_SETTINGS);
    if (configShareMemoryId == -1) {
        safeExit(1);
    }

    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        safeExit(2);
    }

    initializeEnvironment();
    initializeSingalsHandlers();
    if (loadConfig(configShareMemoryId) == -1) {
        safeExit(3);
    }
     
     
    /* ----- ALL ----- */
    acknowledgeInitShareMemoryId = generateShareMemory(sizeof(int) * (configArr[SO_NAVI] + configArr[SO_PORTI]));
    if (acknowledgeInitShareMemoryId == -1) {
        safeExit(10);
    }


    /* ----- ANALYZER ----- */
    analyzerReadingMsgQueue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    analyzerWritingMsgQueue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);

    goodAnalyzerShareMemoryId = generateShareMemory(
        sizeof(goodDailyDump) * configArr[SO_MERCI] * (configArr[SO_NAVI] + configArr[SO_PORTI]));
    if (goodAnalyzerShareMemoryId == -1) {
        safeExit(4);
    }

    boatAnalyzerShareMemoryId = generateShareMemory(sizeof(boatDailyDump) * configArr[SO_NAVI]);
    if (boatAnalyzerShareMemoryId == -1) {
        safeExit(5);
    }

    portAnalyzerShareMemoryId = generateShareMemory(sizeof(portDailyDump) * configArr[SO_PORTI]);
    if (portAnalyzerShareMemoryId == -1) {
        safeExit(6);
    }

    endGoodAnalyzerShareMemoryId = generateShareMemory(sizeof(goodEndDump) * configArr[SO_MERCI]);
    if (endGoodAnalyzerShareMemoryId == -1) {
        safeExit(17);
    }

    if (generateSemaphore(endGoodAnalyzerShareMemoryId) == -1) {
        handleError("Error during creation of semaphore for goods end dump");
        safeExit(18);
    }

    analyzerArgs[0] = configShareMemoryId;
    analyzerArgs[1] = goodAnalyzerShareMemoryId;
    analyzerArgs[2] = boatAnalyzerShareMemoryId;
    analyzerArgs[3] = portAnalyzerShareMemoryId;
    analyzerArgs[4] = analyzerReadingMsgQueue;
    analyzerArgs[5] = analyzerWritingMsgQueue;
    analyzerArgs[6] = endGoodAnalyzerShareMemoryId;
    analyzerArgs[7] = acknowledgeInitShareMemoryId;
    if (generateSubProcesses(1, "./bin/analyzer", 0, analyzerArgs, 8) == -1) {
        safeExit(7);
    }


    /* ----- GOODS ----- */
    goodShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodShareMemoryId == -1) {
        safeExit(8);
    }

    if (initializeGoods(goodShareMemoryId) == -1) {
        safeExit(9);
    }


    /* ----- PORTS ----- */
    portShareMemoryId = generateShareMemory(sizeof(Port) * configArr[SO_PORTI]);
    if (portShareMemoryId == -1) {
        safeExit(11);
    }

    portArgs[0] = configShareMemoryId;
    portArgs[1] = portShareMemoryId;
    portArgs[2] = goodShareMemoryId;
    portArgs[3] = goodAnalyzerShareMemoryId;
    portArgs[4] = portAnalyzerShareMemoryId;
    portArgs[5] = acknowledgeInitShareMemoryId;
    portArgs[6] = endGoodAnalyzerShareMemoryId;
    if (generateSubProcesses(configArr[SO_PORTI], "./bin/porto", 1, portArgs, 7) == -1) {
        safeExit(12);
    }


    /* ----- BOATS ----- */
    boatArgs[0] = configShareMemoryId;
    boatArgs[1] = portShareMemoryId;
    boatArgs[2] = goodAnalyzerShareMemoryId;
    boatArgs[3] = boatAnalyzerShareMemoryId;
    boatArgs[4] = acknowledgeInitShareMemoryId;
    boatArgs[5] = endGoodAnalyzerShareMemoryId;
    if (generateSubProcesses(configArr[SO_NAVI], "./bin/nave", 1, boatArgs, 6) == -1) {
        safeExit(13);
    }


    /* ----- START SIMULATION ----- */
    if (work() == -1) {
        safeExit(15);
    }

    if (cleanup() == -1) {
        safeExit(16);
    }

    return 0;
}

int initializeSingalsHandlers() {

    setpgrp();

    signal(SIGINT, handle_master_stopProcess);

    return 0;
}

int checkForAnalizerToFinish() {

    PortMessage response;

    int msgResponse = receiveMessage(analyzerReadingMsgQueue, &response, 0, 0);
    if (msgResponse == -1) {
        handleError("Error during waiting response from PA_FINISH");
        return -1;
    }

    switch (response.msg.data.action)
    {
        case PA_FINISH:
            break;
        case PA_EOS_GSR:
            printf("No more request or in stock of goods\n");
            simulationFinishedEarly = 1;
            return checkForAnalizerToFinish();
            break;
        default:
            handleError("Wrong action response instead of PA_FINISH");
            return -1;
    }

    return 0;
}

int waitForAnalizerToCollectData() {

    PortMessage response;

    int msgResponse = receiveMessage(analyzerReadingMsgQueue, &response, 0, 0);
    if (msgResponse == -1) {
        handleError("Error during waiting response from PA_DATA_COL");
        return -1;
    }

    if (response.msg.data.action != PA_DATA_COL) {
        handleError("Wrong action response instead of PA_DATA_COL");
        return -1;
    }

    return 0;
}

int acknowledgeChildrenStatus() {

    double waitTimeInSeconds = 0.005;
    int allChildrenInit, i;
    int *acknowledgeArr;
    int entities = configArr[SO_NAVI] + configArr[SO_PORTI];

    acknowledgeArr = (int*) shmat(acknowledgeInitShareMemoryId, NULL, 0);
    if (acknowledgeArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    do
    {
        long waitTimeInNs = getNanoSeconds(waitTimeInSeconds);
        if (safeWait(0, waitTimeInNs) == -1) {
            handleError("Error while waiting init children");
            return -1;
        }

        allChildrenInit = 1;

        for (i = 0; i < entities; i++)
        {
            if (acknowledgeArr[i] == 0) {
                debug("Failed init check");
                allChildrenInit = 0;
                break;
            }
        }

        waitTimeInSeconds += waitTimeInSeconds;

        if (waitTimeInSeconds > 1) {
            handleError("Wait time for init check exeaded 1 second");
            return -1;
        }

    } while (allChildrenInit != 1);

    /* Reset acknowledge */
    memset(acknowledgeArr, 0, entities);

    if (shmdt(acknowledgeArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int work() {

    int simulationDays = 0;

    if (acknowledgeChildrenStatus() == -1) {
        handleError("Error while waiting for children to be initialized");
        return -1;
    }

    /* Set the current group different from itselfe to avoid interupt from custom signals */
    setpgid(getpid(), getppid());

    killpg(getpid(), SIGUSR1);

    while (simulationDays < configArr[SO_DAYS] && simulationFinishedEarly == 0)
    {
        char buffer[128];

        snprintf(buffer, sizeof(buffer), "Day number %d", simulationDays);
        printConsole(buffer);

        simulationDays++;

        if (safeWait(1, 0l) == -1) {
            handleError("Error while waiting next day master");
            return -1;
        }

        if (simulationDays < configArr[SO_DAYS] && simulationFinishedEarly == 0) {

            if (killpg(getpid(), SIGUSR2) == -1) {
                handleErrno("killpg()");
                return -1;
            }

            /* Salta il primo giorno */
            if (simulationDays > 1) {

                checkForAnalizerToFinish();
            }

            if (sendMessage(analyzerWritingMsgQueue, PA_NEW_DAY, -1, -1) == -1) {
                handleError("Error during sendig of the PA_NEW_DAY");
                return -1;
            }

            waitForAnalizerToCollectData();

            if (simulationFinishedEarly == 0) {
                killpg(getpid(), SIGCONT);
            }
        }
    }

    killpg(getpid(), SIGSYS);

    if (acknowledgeChildrenStatus() == -1) {
        handleError("Error while waiting for children to finish");
        safeExit(14);
    }

    if (sendMessage(analyzerWritingMsgQueue, PA_NEW_DAY, -1, -1) == -1) {
        handleError("Error during sendig of the PA_NEW_DAY finish");
        return -1;
    }

    printConsole("Simulation finished");

    return 0;
}

int generateShareMemory(int sizeOfSegment) {
    
    int *sharedMemory;
    int shareMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (shareMemoryId == -1) {
        handleErrno("shmget()");
        return -1;
    }

    sharedMemory = (int*) shmat(shareMemoryId, NULL, 0);
    if (sharedMemory == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    memset(sharedMemory, 0, sizeOfSegment);

    if (shmdt(sharedMemory) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return shareMemoryId;
}

/* Generate a semaphore */
/* https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes */
int generateSemaphore(int semKey) {

    char semaphoreKey[12];
    sem_t *semaphore;

    if (sprintf(semaphoreKey, "%d", semKey) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    return 0;
}

/* Generate processes by forking master and using execve */
/* Return 0 if the processes has been loaded succesfully, -1 if some errors occurred. */
int generateSubProcesses(int nOfProcess, char *execFilePath, int includeProceduralId, int *arguments, int argSize) {
    
    int i, arraySize;    
    char **args;

    if(nOfProcess <= 0) {
        return 0;
    }

    arraySize = argSize + includeProceduralId + 1;
    args = malloc(sizeof(char *) * arraySize);
    for (i = 0; i < arraySize; i++) {
        args[i] = (char *) malloc(sizeof(char) * 12);
    }

    for (i = 0 + includeProceduralId; i < argSize + includeProceduralId; i++) {
        
        char key[12];
    
        if (sprintf(key, "%d", arguments[i - includeProceduralId]) == -1) {
            handleError("Error during conversion of the argument to a string");
            return -1;
        }

        memcpy(args[i], key, sizeof(char) * 12); 
    }
 
    args[arraySize - 1] = '\0';

    i = 0;
    while (i++ < nOfProcess)
    {
        int id = fork();
        
        /* If includeProceduralId == 1 set in the first position the id of the process */
        if (includeProceduralId == 1) {

            char key[12];
        
            if (sprintf(key, "%d", currentProcessId) == -1) {
                handleError("Error during conversion of the id to a string");
                return -1;
            }

            memcpy(args[0], key, sizeof(char) * 12); 
            currentProcessId++;
        }

        if (id == -1) {
            handleError("Error during fork of the file");
            return -1;
        }
        if (id == 0) {  
            if (execve(execFilePath, args, NULL) == -1) {
                handleErrno("execve()");
                return -1;
            }
        } 
    }

    free(args);

    return 0;
}

int initializeGoods(int goodShareMemoryId) {

    /* Init of goods */
    if (generateGoods(goodShareMemoryId) == -1) {
        handleError("Error while generating goods");
        return -1;
    }

    return 0;
}

/* Generate all the goods requested and initialize them */
int generateGoods(int goodShareMemoryId) {

    int *randomGoodDistribution;
    int i;

    Goods *arr = (Goods*) shmat(goodShareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        handleErrno("shmat");
        return -1;
    }

    randomGoodDistribution = (int *) malloc(sizeof(int) * configArr[SO_MERCI]);
    generateSubgroupSums(randomGoodDistribution, configArr[SO_FILL], configArr[SO_MERCI]);

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods good;
        good.id = i;
        /* I got the amount of each port must have */
        good.loadInTon = randomGoodDistribution[i] / configArr[SO_PORTI];
        good.state = Undefined;
        good.remaningDays = getRandomValue(configArr[SO_MIN_VITA], configArr[SO_MAX_VITA]);

        arr[i] = good;
    }

    if (shmdt(arr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    free(randomGoodDistribution);

    return 0;
}

int cleanup() {

    if (shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmctl(configShareMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    if (shmctl(goodShareMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }
    
    if (shmctl(portShareMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("shmctl()");
        return -1;
    }

    debug("Master clean");

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    killpg(getpid(), SIGINT);
    exit(exitNumber);
}