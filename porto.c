#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  

#include "lib/config.h"
#include "lib/utilities.h"
#include "lib/msgPortProtocol.h"

Port port;
/*Goods supplies[]; TODO 
Goods demands[];*/

int main(int argx, char* argv[]) {
    
    if (initiPort(argv[0], argv[1]) == -1) {
        printf("Initialization of port %s failed\n", argv[0]);
        return 1;
    }
    
    return 0;
}

/* Initialize port */
int initiPort(char* portIdS, char* shareMemoryIdS) {

    int portMsgId = msgget(IPC_PRIVATE, 0600);
    if (portMsgId == -1) {
        return 1;
    }

    char* p;
    int portId = strtol(portIdS, &p, 10);

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

    int shareMemoryId = strtol(shareMemoryIdS, &p, 10);
    Coordinates* arr = (Coordinates*) shmat(shareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        return -1;
    }

    arr[port.id] = port.position;

    return 0;
}