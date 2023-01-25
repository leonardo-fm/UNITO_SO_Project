#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  

#include <semaphore.h>
#include <fcntl.h>      

#include "lib/config.h"
#include "lib/utilities.h"
#include "lib/msgPortProtocol.h"

Port port;
GoodExchange goodExchange;

int main(int argx, char* argv[]) {
    
    if (initiPort(argv[0], argv[1], argv[2]) == -1) {
        printf("Initialization of port %s failed\n", argv[0]);
        return 1;
    }
    
    return 0;
}

/* Initialize port */
int initiPort(char* portIdString, char* portShareMemoryIdS, char* goodShareMemoryIdS) {

    if (initializePortStruct(portIdString, portShareMemoryIdS) == -1) {
        printf("Error occurred during init of port struct");
        return -1;
    }

    if (initializeExchangeGoods() == -1) {
        printf("Error occurred during init of goods exchange");
        return -1;
    }

    if (initializePortGoods(goodShareMemoryIdS) == -1) {
        printf("Error occurred during init of goods");
        return -1;
    }

    return 0;
}

int initializePortStruct(char* portIdString, char* portShareMemoryIdS) {
    
    int portMsgId = msgget(IPC_PRIVATE, 0600);
    if (portMsgId == -1) {
        return -1;
    }
    
    char* p;
    int portId = strtol(portIdString, &p, 10);

    port.id = portId;
    port.msgQueuId = portMsgId;
    if (port.id < 4) {
        port.position = getCornerCoordinates(SO_LATO, SO_LATO, port.id);
    }
    else {
        port.position = getRandomCoordinates(SO_LATO, SO_LATO);
    }
    port.quays = getRandomValue(4, SO_BANCHINE);
    port.availableQuays = port.quays;

    int shareMemoryId = strtol(portShareMemoryIdS, &p, 10);
    Coordinates* arr = (Coordinates*) shmat(shareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        return -1;
    }

    arr[port.id] = port.position;

    return 0;
}

int initializeExchangeGoods() {

    int i, j = 0;
    
    goodExchange.exchange = malloc(2 * sizeof(int*));
    if (goodExchange.exchange == NULL) {
        printf("Error during initialize exchange");
        return -1;
    }

    for (i = 0; i < SO_MERCI; i++) {
        goodExchange.exchange[i] = malloc(2 * sizeof(int));
        if (goodExchange.exchange[i] == NULL) {
            printf("Error during initialize exchange");
            return -1;
        }
        for (j = 0; j < 2; j++) {
            goodExchange.exchange[i][j] == 0;
        }
    }
}

int initializePortGoods(char* goodShareMemoryIdS) {

    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", getppid()) == -1) {
        printf("Error during conversion of the pid for semaphore to a string");
        return -1;
    }   

    sem_t *semaphore = sem_open(semaphoreKey, O_CREAT | O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Error semaphore opening");
        return -1;
    }

    int maxTake = SO_FILL / SO_MERCI / SO_PORTI;
    sem_wait(semaphore);



    sem_post(semaphore);

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the semaphore");
        return -1;
    }
}
