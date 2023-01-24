#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  

#include "lib/config.h"
#include "lib/utilities.h"

int currentPortMsgId;

int main(int argx, char* argv[]) {
    
    if (initiPort(argv[0], argv[1]) == -1) {
        printf("Initialization of port %s failed\n", argv[0]);
        return 1;
    }
    
    return 0;
}

int initiPort(char* portIdS, char* shareMemoryIdS) {

    currentPortMsgId = msgget(IPC_PRIVATE, 0600);
    if (currentPortMsgId == -1) {
        return 1;
    }

    char* p;
    int portId = strtol(portIdS, &p, 10);
    int shareMemoryId = strtol(shareMemoryIdS, &p, 10);
    Coordinates* arr = (Coordinates*) shmat(shareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        return -1;
    }
    Coordinates currentCoordinates = getRandomCoordinates(SO_LATO, SO_LATO);
    currentCoordinates.msgQueuId = currentPortMsgId;
    arr[portId] = currentCoordinates;

    return 0;
}