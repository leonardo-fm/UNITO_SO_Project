typedef enum {
    PA_Y,           /* Yes */
    PA_N,           /* No */
    PA_SETUP,       /* Create a queue for read and a queue for write */
    PA_ACCEPT,      /* Can i use a quay? */   
    PA_SE_GOOD,     /* Sell good */
    PA_SE_SUMMARY,  /* Sell good summary */
    PA_RQ_GOOD,     /* Request good */
    PA_RQ_SUMMARY,  /* Request good summary */
    PA_EOT,         /* End of trasmission */ 
    PA_NEW_DAY,     /* New day for analyzer */
    PA_DATA_COL,    /* Data collected from boats and ports */    
    PA_FINISH       /* Analyzer finished analyzing data */
} ProtocolActions;

typedef struct {
    long int msgType;
    union {
        char msg[sizeof(int) * 3];
        struct {
            ProtocolActions action;
            /* Use for shared memory id / queue id / summary */
            int data1;
            /* Use for semaphore key / queue id */
            int data2;
        } data;
    } msg;
} PortMessage;

extern int stopWaitingQueues;

int sendMessage(int msgQueueId, ProtocolActions action, int data1, int data2);
int receiveMessage(int msgQueueId, PortMessage *pMsg, int flag, int forceStop); 