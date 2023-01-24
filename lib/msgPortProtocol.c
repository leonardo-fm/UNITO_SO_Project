#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   
#include <memory.h>  
#include <sys/shm.h>

#include "msgPortProtocol.h"

int MAX_BUFFER_PORT_MSG = sizeof(int) * 3;

int sendMessage(int msgQueuId, int boatId, ProtocolActions action, int quantity) {
    
    PortMessage msg;
    msg.msgType = 1;
    msg.msg.data.id = boatId;
    msg.msg.data.action = action;
    msg.msg.data.quantity = quantity;

    if (msgsnd(msgQueuId, &msg, MAX_BUFFER_PORT_MSG, 0) == -1) {
        printf("Error during sending of the message");
        return -1;
    }

    return 0;
}

PortMessage reciveMessage(int boatId) {

} 
