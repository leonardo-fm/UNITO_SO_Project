#define _GNU_SOURCES
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>   
#include <signal.h>  
#include <errno.h>

#include <semaphore.h>
#include <fcntl.h>    

#include <sys/shm.h>       

#include "lib/config.h"
#include "lib/utilities.h"

int NUM_OF_SETTINGS = 13;
int* configArr;

int main(int argx, char* argv[]) {

    /* ----- CONFIG ----- */
    int configShareMemoryId = generateShareMemory(sizeof(int) * NUM_OF_SETTINGS);
    if (configShareMemoryId == -1) {
        printf("Error during creation of shared memory for config\n");
        exit(1);
    }

    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        exit(2);
    }

    initializeEnvironment();
    if (loadConfig(configShareMemoryId) == -1) {
        exit(3);
    }


    /* ----- GOODS ----- */
    int goodShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodShareMemoryId == -1) {
        printf("Error during creation of shared memory for goods\n");
        exit(4);
    }

    if (initializeGoods(goodShareMemoryId) == -1) {
        exit(5);
    }


    /* ----- PORTS ----- */
    int portShareMemoryId = generateShareMemory(sizeof(Coordinates) * configArr[SO_PORTI]);
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
 
    /* Avoid semaphore error creation */
    sleep(1);

    /* Array of pointers of shared memory segment */
    int arrOfPointerId[] = {configShareMemoryId, goodShareMemoryId, portShareMemoryId}; 
    if (cleanup(arrOfPointerId, sizeof(arrOfPointerId) / sizeof(int)) == -1) {
        printf("Cleanup failed\n");
        exit(9);
    }

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
int generateSubProcesses(int nOfProcess, char* execFilePath, int configShareMemoryId, int portShareMemoryId, int goodShareMemoryId) {
    
    if(nOfProcess <= 0) {
        return 0;
    }

    int i = 0;
    while (i++ < nOfProcess)
    {
        int id = fork();
        if (id == -1) {
            printf("Error during fork of the file %s\n", execFilePath);
            return -1;
        }
        if (id == 0) {  

            char idS[12];
            /* i - 1 because i need to have the ids to start from 0 to n */
            if (sprintf(idS, "%d", (i - 1)) == -1) {
                printf("Error during conversion of the id to a string\n");
                return -1;
            }

            char configShareMemoryIdString[12];
            if (sprintf(configShareMemoryIdString, "%d", configShareMemoryId) == -1) {
                printf("Error during conversion of the config id of shared memory to a string\n");
                return -1;
            }

            char portShareMemoryIdString[12];
            if (sprintf(portShareMemoryIdString, "%d", portShareMemoryId) == -1) {
                printf("Error during conversion of the port id of shared memory to a string\n");
                return -1;
            }

            char goodShareMemoryIdString[12];
            if (sprintf(goodShareMemoryIdString, "%d", goodShareMemoryId) == -1) {
                printf("Error during conversion of the good id of shared memory to a string\n");
                return -1;
            }

            char* arr[] = {configShareMemoryIdString, idS, portShareMemoryIdString, goodShareMemoryIdString, NULL};

            if (execve(execFilePath, arr, NULL) == -1) {
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
}

/* Generate all the goods requested and initialize them */
int generateGoods(int goodShareMemoryId) {

    Goods* arr = (Goods*) shmat(goodShareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        printf("Error during good memory allocation\n");
        return -1;
    }

    int goodQuantity = (configArr[SO_FILL] / 2) / configArr[SO_MERCI];
    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods good;
        good.id = i;
        good.loadInTon = goodQuantity;
        good.state = Undefined;
        good.remaningDays = getRandomValue(configArr[SO_MIN_VITA], configArr[SO_MAX_VITA]);

        arr[i] = good;
    }

    return 0;
}

/* Generate a semaphore */
/* https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes */
int generateSemaphore() {

    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", getpid()) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    sem_t* semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
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

int cleanup(int pointerIdArr[], int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (shmctl(pointerIdArr[i], IPC_RMID, NULL) == -1) {
            printf("The shared memory failed to be closed\n");
            return -1;
        }
    }
}