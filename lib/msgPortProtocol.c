#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   
#include <memory.h>  
#include <sys/shm.h>
#include <sys/msg.h>

#include "msgPortProtocol.h"

int MAX_BUFFER_PORT_MSG = sizeof(int) * 4;

/* Send a message to the queue, eturn 0 if ok otherwise -1 */
int sendMessage(int msgQueueId, int boatId, ProtocolActions action, int sharedMemoryId, int semaphoreKey) {
    
    PortMessage pMsg;
    pMsg.msgType = 1;
    pMsg.msg.data.id = boatId;
    pMsg.msg.data.action = action;
    pMsg.msg.data.sharedMemoryId = sharedMemoryId;
    pMsg.msg.data.semaphoreKey = semaphoreKey;

    if (msgsnd(msgQueueId, &pMsg, MAX_BUFFER_PORT_MSG, 0) == -1) {
        printf("Error during sending of the message");
        return -1;
    }

    return 0;
}

/* Set the message with the first message in the queue, if the id match, otherwise set NULL */
/* Return 0 if ok, -1 if some error occurred and -2 if no message was found with the given id */
int reciveMessageById(int msgQueueId, int boatId, PortMessage* pMsg) {

    long int msgToRec = 0;

    if (msgrcv(msgQueueId, &pMsg, MAX_BUFFER_PORT_MSG, msgToRec, 0) == -1) {
        printf("Error during retriving the message");
        return -1;
    }

    if (pMsg->msg.data.id != boatId) {
        pMsg = NULL;
        return -2;
    } else {
        msgctl(msgQueueId, IPC_RMID, 0);
        return 0;
    }
} 

/* Set the message with the first message in the queue */
/* Return 0 if ok, -1 if some error occurred and -2 if no message was found */
int reciveMessage(int msgQueueId, PortMessage* pMsg) {

    long int msgToRec = 0;

    if (msgrcv(msgQueueId, &pMsg, MAX_BUFFER_PORT_MSG, msgToRec, 0) == -1) {
        printf("Error during retriving the message");
        return -1;
    }

    if ((int) strlen(pMsg->msg.msg) != MAX_BUFFER_PORT_MSG) {
        return -2;
    } else {
        msgctl(msgQueueId, IPC_RMID, 0);
        return 0;
    }
} 
