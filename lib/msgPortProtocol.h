typedef enum {
    PA_Y,           /* Yes */
    PA_N,           /* No */
    PA_SETUP,       /* Create a queue for read and a queue for write */
    PA_ACCEPT,      /* Can i use a quay? */   
    PA_SE_GOOD,     /* Sell good */
    PA_RQ_GOOD,     /* Request good */
    PA_EOT          /* End of trasmission */     
} ProtocolActions;

typedef struct {
    long int msgType;
    union {
        char msg[sizeof(int) * 3];
        struct {
            ProtocolActions action;
            /* Use for shared memory id / queue id */
            int data1;
            /* Use for semaphore key / queue id */
            int data2;
        } data;
    } msg;
} PortMessage;

extern int stopWaitingQueues;

int sendMessage(int msgQueueId, ProtocolActions action, int data1, int data2);
int receiveMessage(int msgQueueId, PortMessage *pMsg, int flag, int forceStop); 