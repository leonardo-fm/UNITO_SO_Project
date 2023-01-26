#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    
#include <sys/shm.h>   

#include "lib/utilities.h"
#include "lib/config.h"
#include "lib/msgPortProtocol.h"

int NUM_OF_SETTINGS = 13;
int* configArr;

int sharedMemoryPointer; 
int currentMsgQueueId;
Boat boat;
Goods* goodHold;

int main(int argx, char* argv[]) {

    initializeEnvironment();

    if (initializeConfig(argv[0]) == -1) {
        printf("Initialization of boat config failed\n");
        exit(1);
    }

    if (initializeBoat(argv[1], argv[2]) == -1) {
        printf("Initialization of boat %s failed\n", argv[0]);
        exit(2);
    }

    if (cleanup() == -1) {
        printf("Cleanup failed\n");
        exit(3);
    }

    return 0;
}

int initializeConfig(char* configShareMemoryIdString) {

    char* p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        return -1;
    }

    return 0;
}

int initializeBoat(char* boatIdS, char* shareMemoryIdS) {

    char* p;
    boat.id = strtol(boatIdS, &p, 10);
    boat.position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    boat.capacityInTon = configArr[S0_CAPACITY];
    boat.speed = configArr[SO_SPEED];
    boat.state = In_Sea;

    sharedMemoryPointer = strtol(shareMemoryIdS, &p, 10);

    /* Initialization of the hold */
    goodHold = malloc(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodHold == NULL) {
        printf("Error during malloc of boat hold\n");
        return -1;
    }

    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods emptyGood;
        emptyGood.id = i;
        emptyGood.loadInTon = 0;
        emptyGood.state = In_The_Boat;

        goodHold[i] = emptyGood;
    }
    
    return 0;
}

int openComunication(int portId) {

    if (currentMsgQueueId != -1) {
        printf("The old comunication with id %d was not closed properly\n", currentMsgQueueId);
        return -1;
    }

    currentMsgQueueId = msgget((key_t) portId, IPC_CREAT | 0600);
    if (sendMessage(currentMsgQueueId, boat.id, PA_ACCEPT, 0, 0) == -1) {
        printf("Failed to send ACCEPT comunication\n");
        return -1;
    }

    /* TODO Aspettare risposta e sapere se andare in dialougue() oppure nela FIFO */

    return 0;
}

int trade() {

    int treading = 1;
    
    while (treading == 1) {
        if (haveIGoodsToSell() == 0) {
            /* Sell goods */

        } else {
            /* Buy goods */

        }
    }

    return 0;
}

int closeComunication() {

    if (sendMessage(currentMsgQueueId, boat.id, PA_EOT, 0, 0) == -1) {
        printf("Failed to send EOT comunication\n");
        return -1;
    }

    currentMsgQueueId = -1;

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

int cleanup() {
    
    return 0;
}