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

/*#define CHECK(x) ({
    if (x == -1) {
        fprintf(stderr, "ERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        exit(-1);
    }
});*/

void cleanUp(int num) {

    cleanEnvironment();
}

int main(int argx, char* argv[]) {

    signal(SIGINT, cleanUp);
    signal(SIGTERM, cleanUp);

    initializeEnvironment();
    if (loadConfig() == -1) {
        return 1;
    }

    int portShareMemoryId = generateShareMemory(sizeof(Coordinates) * SO_PORTI);
    if (portShareMemoryId == -1) {
        printf("Error during creation of shared memory of ports");
        return 2;
    }

    int goodShareMemoryId = generateShareMemory(sizeof(Goods) * SO_MERCI);
    if (goodShareMemoryId == -1) {
        printf("Error during creation of shared memory of goods");
        return 3;
    }

    /* Generation of semapore for goods init */
    if (generateSubProcesses(SO_PORTI, "./bin/porto", portShareMemoryId, goodShareMemoryId) == -1) {
        return 4;
    }

    /* Generation of "porto" process */
    if (generateSubProcesses(SO_PORTI, "./bin/porto", portShareMemoryId, goodShareMemoryId) == -1) {
        return 4;
    }

    /* Generation of "nave" process */
    if (generateSubProcesses(SO_NAVI, "./bin/nave", portShareMemoryId, 0) == -1) {
        return 5;
    }

    return 0;
}

int generateShareMemory(int sizeOfSegment) {
    int shareMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (shareMemoryId == -1) {
        printf("Error during creation of the shared memory");
        return -1;
    }

    return shareMemoryId;
}

/* Generate processes by forking master and using execve */
/* Return 0 if the processes has been loaded succesfully, -1 if some errors occurred. */
int generateSubProcesses(int nOfProcess, char* execFilePath, int portShareMemoryId, int goodShareMemoryId) {

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
                printf("Error during conversion of the id to a string");
                return -1;
            }

            char portShareMemoryIdString[12];
            if (sprintf(portShareMemoryIdString, "%d", portShareMemoryId) == -1) {
                printf("Error during conversion of the port id of shared memory to a string");
                return -1;
            }

            char goodShareMemoryIdString[12];
            if (sprintf(goodShareMemoryIdString, "%d", goodShareMemoryId) == -1) {
                printf("Error during conversion of the good id of shared memory to a string");
                return -1;
            }

            char* arr[] = { idS, portShareMemoryIdString, goodShareMemoryIdString, NULL };

            if (execve(execFilePath, arr, NULL) == -1) {
                printf("Error during innesting of the file %s\n", execFilePath);
                return -1;
            }
        } 
    }

    return 0;
}

int generateGoods(int goodShareMemoryId) {

    Goods* arr = (Goods*) shmat(goodShareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        printf("Error during good initialization");
        return -1;
    }

    int goodQuantity = SO_FILL / SO_MERCI;
    int i = 0;
    for (i = 0; i < SO_MERCI; i++) {
        Goods good;
        good.id = i;
        good.loadInTon = goodQuantity;
        good.state = Undefined;
        int lifeSpan = getRandomValue(SO_MIN_VITA, SO_MAX_VITA);
        good.lifespan = lifeSpan;
        good.remaningDays = lifeSpan;

        arr[i] = good;
    }

    return 0;
}

/* Generate a semaphore */
/* https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes */
int generateSemaphore() {

    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", getpid()) == -1) {
        printf("Error during conversion of the pid for semaphore to a string");
        return -1;
    }   

    sem_t* semaphore = sem_open(semaphoreKey, O_CREAT | O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Error semaphore opening");
        return -1;
    }

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the semaphore");
        return -1;
    }

    return 0;
}
