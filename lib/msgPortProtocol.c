#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h>  
#include <memory.h>  
#include <sys/shm.h>
#include <sys/msg.h>

#include "msgPortProtocol.h"

size_t MAX_BUFFER_PORT_MSG = sizeof(int) * 5;

/* Send a message to the queue, eturn 0 if ok otherwise -1 */
int sendMessage(int msgQueueId, int boatId, Receiver receiver, ProtocolActions action, int sharedMemoryId, int semaphoreKey) {
    
    printf("Sending message in queue id: %d, with action: %d\n", msgQueueId, action);
    PortMessage pMsg;
    pMsg.msgType = 1;
    pMsg.msg.data.id = boatId;
    pMsg.msg.data.receiver = receiver;
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
int reciveMessageById(int msgQueueId, int boatId, Receiver receiver, PortMessage* pMsg) {

    long int msgToRec = 0;
    printf("Reading msgQueue with id: %d\n", msgQueueId);
    if (msgrcv(msgQueueId, pMsg, MAX_BUFFER_PORT_MSG, msgToRec, 0) == -1) {
        /* https://stackoverflow.com/questions/23371855/check-a-msqid-to-see-if-there-is-message-without-waiting-or-msgrcv */
        if (errno == ENOMSG) {
            /* No message in the queue */
            return -2;
        }
        printf("Error during retriving the message\n");
        return -1;
    }
    printf("Outside id, boat id: %d\n", pMsg->msg.data.id);
    if (pMsg->msg.data.id == boatId && pMsg->msg.data.receiver == receiver) {
        msgctl(msgQueueId, IPC_RMID, 0);
        return 0;
    } else {
        return -2;
    }
} 

/* Set the message with the first message in the queue */
/* Return 0 if ok, -1 if some error occurred and -2 if no message was found */
int reciveMessage(int msgQueueId, Receiver receiver, PortMessage* pMsg) {

    long int msgToRec = 0;
    printf("Reading msgQueue %d\n", msgQueueId);
    if (msgrcv(msgQueueId, pMsg, MAX_BUFFER_PORT_MSG, msgToRec, 0) == -1) {
        printf("Inside error no id\n");
        if (errno == ENOMSG) {
            /* No message in the queue */
            return -2;
        }
        printf("Error during retriving the message, errno:%d\n", errno);
        return -1;
    }
    printf("Outside\n");
    if (pMsg->msg.data.receiver == receiver) {
        msgctl(msgQueueId, IPC_RMID, 0);
        return 0;
    } else {
        return -2;
    }
} 
