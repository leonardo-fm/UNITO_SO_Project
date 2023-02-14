#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h>  
#include <memory.h>  
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>

#include "msgPortProtocol.h"

int stopWaitingQueues = 0;

size_t MAX_BUFFER_PORT_MSG = sizeof(int) * 3;

/* Send a message to the queue, eturn 0 if ok otherwise -1 */
int sendMessage(int msgQueueId, ProtocolActions action, int data1, int data2) {
    
    PortMessage pMsg;
    pMsg.msgType = 1;
    pMsg.msg.data.action = action;
    pMsg.msg.data.data1 = data1;
    pMsg.msg.data.data2 = data2;

    if (msgsnd(msgQueueId, &pMsg, MAX_BUFFER_PORT_MSG, 0) == -1) {
        printf("Error during sending of the message errno:%d\n", errno);

        /* Check if the msg queue exist */
        if (kill(msgQueueId, 0) == 0) {
            printf("The msg queue %d exist\n", msgQueueId);
        } else {
            printf("The msg queue %d not exist\n", msgQueueId);
        }

        return -1;
    }

    return 0;
}

/* Set the message with the first message in the queue */
/* Return 0 if ok, -1 if some error occurred and -2 if no message was found */
int receiveMessage(int msgQueueId, PortMessage *pMsg, int flag) {

    int msgStatus;
    long int msgToRec = 0;
    
    memset(pMsg, 0, sizeof(PortMessage));

    do {
    
        /* Reset errno error */
        errno = 0;
        msgStatus = msgrcv(msgQueueId, pMsg, MAX_BUFFER_PORT_MSG, msgToRec, flag);

    } while (msgStatus == -1 && errno == EINTR);

    if (errno == ENOMSG) {
        /* No message in the queue */
        return -2;
    } else if (errno != EINTR && errno != 0) {
        printf("Error during retrieving the message from msg queue: %d (swq: %d), errno: %d\n", msgQueueId, stopWaitingQueues, errno);
        /* Check if the msg queue exist */
        if (kill(msgQueueId, 0) == 0) {
            printf("The msg queue %d exist\n", msgQueueId);
        } else {
            printf("The msg queue %d not exist\n", msgQueueId);
        }
        
        return -1;
    }

    return 0;
} 
