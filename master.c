#define _GNU_SOURCES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>    
#include <sys/shm.h>    
#include <signal.h> 

#include "lib/config.h"
#include "lib/utilities.h"

#include "master.h"

int configShareMemoryId;
int goodShareMemoryId;
int portShareMemoryId;

int NUM_OF_SETTINGS = 13;
int *configArr;

void handle_master_stopProcess() { 

    printf("\nStopping program...\n");
    killpg(getpid(), SIGINT);
    cleanup();
    exit(0);
}

int main() {

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

    /* ----- GOODS ----- */
    goodShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodShareMemoryId == -1) {
        printf("Error during creation of shared memory for goods\n");
        exit(4);
    }

    if (initializeGoods(goodShareMemoryId) == -1) {
        exit(5);
    }


    /* ----- PORTS ----- */
    portShareMemoryId = generateShareMemory(sizeof(Port) * configArr[SO_PORTI]);
    if (portShareMemoryId == -1) {
        printf("Error during creation of shared memory for ports\n");
        exit(6);
    }

    if (generateSubProcesses(configArr[SO_PORTI], "./bin/porto", configShareMemoryId, portShareMemoryId, goodShareMemoryId) == -1) {
        exit(7);
    }


    /* ----- BOATS ----- */
    if (generateSubProcesses(configArr[SO_NAVI], "./bin/nave", configShareMemoryId, portShareMemoryId, 0) == -1) {
        exit(8);
    }

sleep(1);
    /* ----- START SIMULATION ----- */
    if (work() == -1) {
        printf("Error during master work\n");
        exit(9);
    }

    if (cleanup() == -1) {
        printf("Cleanup failed\n");
        exit(10);
    }

    return 0;
}

int initializeSingalsHandlers() {

    setpgrp();

    signal(SIGINT, handle_master_stopProcess);

    return 0;
}

int work() {

    int simulationDays = configArr[SO_DAYS];

    /* Set the current group different from itselfe to avoid interupt from custom signals */
    setpgid(getpid(), getppid());

    killpg(getpid(), SIGUSR1);

    while (simulationDays > 0)
    {
        printf("Remaning days %d\n", simulationDays);
        simulationDays--;

        if (safeWait(1, 0l) == -1) {
            printf("Error while waiting next day master\n");
            return -1;
        }

        if (simulationDays > 0) {
            killpg(getpid(), SIGUSR2);
        }
    }

    killpg(getpid(), SIGSYS);
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
int generateSubProcesses(int nOfProcess, char *execFilePath, int configShareMemoryId, int portShareMemoryId, int goodShareMemoryId) {
    
    int i = 0;    

    if(nOfProcess <= 0) {
        return 0;
    }

    while (i++ < nOfProcess)
    {
        int id = fork();
        if (id == -1) {
            printf("Error during fork of the file %s\n", execFilePath);
            return -1;
        }
        if (id == 0) {  

            char *args[5];

            char configShareMemoryIdString[12];
            char idS[12];
            char portShareMemoryIdString[12];
            char goodShareMemoryIdString[12];
            
            if (sprintf(configShareMemoryIdString, "%d", configShareMemoryId) == -1) {
                printf("Error during conversion of the config id of shared memory to a string\n");
                return -1;
            }
            args[0] = configShareMemoryIdString;

            /* i - 1 because i need to have the ids to start from 0 to n */
            if (sprintf(idS, "%d", (i - 1)) == -1) {
                printf("Error during conversion of the id to a string\n");
                return -1;
            }
            args[1] = idS;

            if (sprintf(portShareMemoryIdString, "%d", portShareMemoryId) == -1) {
                printf("Error during conversion of the port id of shared memory to a string\n");
                return -1;
            }
            args[2] = portShareMemoryIdString;

            if (sprintf(goodShareMemoryIdString, "%d", goodShareMemoryId) == -1) {
                printf("Error during conversion of the good id of shared memory to a string\n");
                return -1;
            }
            args[3] = goodShareMemoryIdString;
            
            args[4] = NULL;

            if (execve(execFilePath, args, NULL) == -1) {
                printf("Error during innesting of the file %s\n", execFilePath);
                return -1;
            }
        } 
    }

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