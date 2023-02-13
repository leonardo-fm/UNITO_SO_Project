#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h>  
#include <memory.h>  
#include <sys/shm.h>
#include <sys/msg.h>

#include "msgPortProtocol.h"

size_t MAX_BUFFER_PORT_MSG = sizeof(int) * 3;
int runningStatus = 1;

/* Set the status of the simulation */
void setRunningStatus(int status) {
    printf("=== Setted status to %d ===\n", status);
    runningStatus = status;
}

/* Send a message to the queue, eturn 0 if ok otherwise -1 */
int sendMessage(int msgQueueId, ProtocolActions action, int data1, int data2) {
    
    PortMessage pMsg;
    pMsg.msgType = 1;
    pMsg.msg.data.action = action;
    pMsg.msg.data.data1 = data1;
    pMsg.msg.data.data2 = data2;

    if (msgsnd(msgQueueId, &pMsg, MAX_BUFFER_PORT_MSG, 0) == -1 && runningStatus == 1) {
        printf("Error during sending of the message errno:%d\n", errno);
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
        
        if (runningStatus == 0) {
            break;
        }

    } while (msgStatus == -1 && errno == EINTR);

    if (errno == ENOMSG) {
        /* No message in the queue */
        return -2;
    } else if (errno != EINTR && errno != 0 && runningStatus == 1) {
        printf("Error during retrieving the message, errno: %d\n", errno);
        return -1;
    }

    return 0;
} 
