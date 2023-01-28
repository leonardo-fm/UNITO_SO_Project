typedef enum {
    PA_Y,           /* Yes */
    PA_N,           /* No */
    PA_ACCEPT,      /* Can i use a quay? */   
    PA_SE_GOOD,     /* Sell good */
    PA_RQ_GOOD,     /* Request good */
    PA_EOT,         /* End of trasmission */     
} ProtocolActions;

typedef enum {
    BOAT_RECEIVER,
    PORT_RECEIVER,
} Receiver;

typedef struct {
    long int msgType;
    union {
        char msg[sizeof(int) * 5];
        struct {
            int id;
            ProtocolActions action;
            Receiver receiver;
            int sharedMemoryId;
            int semaphoreKey;
        } data;
    } msg;
} PortMessage;

int sendMessage(int msgQueueId, int boatId, Receiver receiver, ProtocolActions action, int sharedMemoryId, int semaphoreKey);
int reciveMessageById(int msgQueueId, int boatId, Receiver receiver, PortMessage* pMsg); 
int reciveMessage(int msgQueueId, Receiver receiver, PortMessage* pMsg); 