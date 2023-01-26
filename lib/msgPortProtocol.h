extern int MAX_BUFFER_PORT_MSG;

typedef enum {
    PA_Y,           /* Yes */
    PA_N,           /* No */
    PA_ACCEPT,      /* Can i use a quay? */   
    PA_SE_GOOD,     /* Sell good */
    PA_RQ_GOOD,     /* Request good */
    PA_EOT,         /* End of trasmission */     
} ProtocolActions;

typedef struct {
    long int msgType;
    union {
        char msg[sizeof(int) * 4];
        struct {
            int id;
            ProtocolActions action;
            int sharedMemoryId;
            int semaphoreKey;
        } data;
    } msg;
} PortMessage;

int sendMessage(int msgQueueId, int boatId, ProtocolActions action, int sharedMemoryId, int semaphoreKey);
int reciveMessageById(int msgQueueId, int boatId, PortMessage* pMsg); 
int reciveMessage(int msgQueueId, PortMessage* pMsg); 