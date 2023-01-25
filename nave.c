#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    
#include <sys/shm.h>   

#include "lib/utilities.h"
#include "lib/config.h"
#include "lib/msgPortProtocol.h"

int sharedMemoryPointer; 
int currentMsgQueueId;
Boat boat;

int main(int argx, char* argv[]) {

    if (initBoat(argv[0], argv[1]) == -1) {
        printf("Initialization of boat %s failed\n", argv[0]);
        return 1;
    }

    return 0;
}

/* Initialize bot */
int initBoat(char* boatIdS, char* shareMemoryIdS) {

    char* p;
    boat.id = strtol(boatIdS, &p, 10);
    boat.position = getRandomCoordinates(SO_LATO, SO_LATO);
    boat.capacityInTon = S0_CAPACITY;
    boat.speed = SO_SPEED;
    boat.state = In_Sea;

    sharedMemoryPointer = strtol(shareMemoryIdS, &p, 10);
    
    return 0;
}

int openComunication(int portId) {

    if (currentMsgQueueId != -1) {
        printf("The old comunication with id %d was not closed properly\n", currentMsgQueueId);
        return -1;
    }

    currentMsgQueueId = msgget((key_t) portId, IPC_CREAT | 0600);
    if (sendMessage(currentMsgQueueId, boat.id, PA_ACCEPT, 0, 0) == -1) {
        printf("Failed to send ACCEPT comunication");
        return -1;
    }

    /* TODO Aspettare risposta e sapere se andare in dialougue() oppure nela FIFO */

    return 0;
}

int dialogue() {

    return 0;
}

int closeComunication() {

    if (sendMessage(currentMsgQueueId, boat.id, PA_EOT, 0, 0) == -1) {
        printf("Failed to send EOT comunication");
        return -1;
    }

    currentMsgQueueId = -1;

    return 0;
}