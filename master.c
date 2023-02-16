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

#include "lib/config.h"
#include "lib/utilities.h"

#include "lib/master.h"

int configShareMemoryId;
int goodShareMemoryId;
int portShareMemoryId;

int goodAnalyzerShareMemoryId;
int boatAnalyzerShareMemoryId;
int portAnalyzerShareMemoryId;

int NUM_OF_SETTINGS = 13;
int *configArr;
int currentProcessId;
int analyzerFinished = 1;

void handle_analyzer() {

    analyzerFinished = 1;
}

void handle_master_stopProcess() { 

    printf("\nStopping program...\n");
    killpg(getpid(), SIGINT);
    cleanup();
    exit(0);
}

int main() {

    int analyzerArgs[4];
    int portArgs[5];
    int boatArgs[4];

    setpgid(getpid(), getpid());

    /* ----- CONFIG ----- */
    configShareMemoryId = generateShareMemory(sizeof(int) * NUM_OF_SETTINGS);
    if (configShareMemoryId == -1) {
        printf("Error during creation of shared memory for config\n");
        exit(1);
    }

    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        exit(2);
    }

    initializeEnvironment();
    initializeSingalsHandlers();
    if (loadConfig(configShareMemoryId) == -1) {
        exit(3);
    }

    /* ----- ANALYZER ----- */
    goodAnalyzerShareMemoryId = generateShareMemory(
        sizeof(goodDailyDump) * configArr[SO_MERCI] * (configArr[SO_NAVI] + configArr[SO_PORTI]));
    if (goodAnalyzerShareMemoryId == -1) {
        printf("Error during creation of shared memory for good analyzer\n");
        exit(4);
    }

    boatAnalyzerShareMemoryId = generateShareMemory(sizeof(boatDailyDump) * configArr[SO_NAVI]);
    if (boatAnalyzerShareMemoryId == -1) {
        printf("Error during creation of shared memory for boat analyzer\n");
        exit(5);
    }

    portAnalyzerShareMemoryId = generateShareMemory(sizeof(portDailyDump) * configArr[SO_PORTI]);
    if (portAnalyzerShareMemoryId == -1) {
        printf("Error during creation of shared memory for port analyzer\n");
        exit(6);
    }

    analyzerArgs[0] = configShareMemoryId;
    analyzerArgs[1] = goodAnalyzerShareMemoryId;
    analyzerArgs[2] = boatAnalyzerShareMemoryId;
    analyzerArgs[3] = portAnalyzerShareMemoryId;
    if (generateSubProcesses(1, "./bin/analyzer", 0, analyzerArgs, 4) == -1) {
        exit(7);
    }


    /* ----- GOODS ----- */
    goodShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodShareMemoryId == -1) {
        printf("Error during creation of shared memory for goods\n");
        exit(8);
    }

    if (initializeGoods(goodShareMemoryId) == -1) {
        exit(9);
    }


    /* ----- PORTS ----- */
    portShareMemoryId = generateShareMemory(sizeof(Port) * configArr[SO_PORTI]);
    if (portShareMemoryId == -1) {
        printf("Error during creation of shared memory for ports\n");
        exit(10);
    }

    portArgs[0] = configShareMemoryId;
    portArgs[1] = portShareMemoryId;
    portArgs[2] = goodShareMemoryId;
    portArgs[3] = goodAnalyzerShareMemoryId;
    portArgs[4] = portAnalyzerShareMemoryId;
    if (generateSubProcesses(configArr[SO_PORTI], "./bin/porto", 1, portArgs, 5) == -1) {
        exit(11);
    }


    /* ----- BOATS ----- */
    boatArgs[0] = configShareMemoryId;
    boatArgs[1] = portShareMemoryId;
    boatArgs[2] = goodAnalyzerShareMemoryId;
    boatArgs[3] = boatAnalyzerShareMemoryId;
    if (generateSubProcesses(configArr[SO_NAVI], "./bin/nave", 1, boatArgs, 4) == -1) {
        exit(12);
    }

    /* TODO removit */
    sleep(1);

    /* ----- START SIMULATION ----- */
    if (work() == -1) {
        printf("Error during master work\n");
        exit(13);
    }

    if (cleanup() == -1) {
        printf("Cleanup failed\n");
        exit(14);
    }

    return 0;
}

int initializeSingalsHandlers() {

    setpgrp();

    signal(SIGALRM, handle_analyzer);
    signal(SIGINT, handle_master_stopProcess);

    return 0;
}

int waitForAnalizerToFinish() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGALRM);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int waitForAnalizerToCollectData() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGTTIN);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int work() {

    int simulationDays = 0;

    /* Set the current group different from itselfe to avoid interupt from custom signals */
    setpgid(getpid(), getppid());

    killpg(getpid(), SIGUSR1);

    while (simulationDays < configArr[SO_DAYS])
    {
        if (analyzerFinished != 1) {
            waitForAnalizerToFinish();
        }

        printf("Day number %d\n", simulationDays);
        simulationDays++;

        if (safeWait(1, 0l) == -1) {
            printf("Error while waiting next day master\n");
            return -1;
        }

        printf("Send SIGUR2...\nSending in group %d\n", getpid());
        killpg(getpid(), SIGUSR2);

        if (simulationDays < configArr[SO_DAYS]) {

            waitForAnalizerToCollectData();
            analyzerFinished = 0;

            killpg(getpid(), SIGCONT);
        }
    }

    killpg(getpid(), SIGSYS); /*Si svegliano? essendo che i porti e navi stano aspettando un SIGCONT*/
    printf("Simulation finished\n");

    return 0;
}

int generateShareMemory(int sizeOfSegment) {
    int shareMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (shareMemoryId == -1) {
        printf("Error during creation of the shared memory\n");
        return -1;
    }

    return shareMemoryId;
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
            printf("Error during conversion of the argument %d° to a string\n", i);
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
                printf("Error during conversion of the id to a string\n");
                return -1;
            }

            memcpy(args[0], key, sizeof(char) * 12); 
            currentProcessId++;
        }

        if (id == -1) {
            printf("Error during fork of the file %s\n", execFilePath);
            return -1;
        }
        if (id == 0) {  
            if (execve(execFilePath, args, NULL) == -1) {
                printf("Error during innesting of the file %s\n", execFilePath);
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
        return -1;
    }

    /* Init of semaphore */
    if (generateSemaphore() == -1) {
        return -1;
    }

    return 0;
}

/* Generate all the goods requested and initialize them */
int generateGoods(int goodShareMemoryId) {

    int goodQuantity, i;

    Goods *arr = (Goods*) shmat(goodShareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        printf("Error during good memory allocation\n");
        return -1;
    }

    goodQuantity = (int)((configArr[SO_FILL] / 2) / configArr[SO_MERCI]);
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods good;
        good.id = i;
        good.loadInTon = goodQuantity;
        good.state = Undefined;
        good.remaningDays = getRandomValue(configArr[SO_MIN_VITA], configArr[SO_MAX_VITA]);

        arr[i] = good;
    }

    if (shmdt(arr) == -1) {
        printf("The init good arr detach failed\n");
        return -1;
    }

    return 0;
}

/* Generate a semaphore */
/* https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes */
int generateSemaphore() {

    sem_t *semaphore;

    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", getpid()) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Error on opening the semaphore\n");
        return -1;
    }

    if (sem_close(semaphore) < 0) {
        printf("Error on closing the semaphore\n");
        return -1;
    }

    return 0;
}

int cleanup() {

    if (shmctl(configShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }

    if (shmctl(goodShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }
    
    if (shmctl(portShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }

    return 0;
}