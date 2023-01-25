extern int MAX_BUFFER_PORT_MSG;

typedef enum {
    PA_Y,           /* Yes */
    PA_N,           /* No */       
    PA_ACCEPT,      /* Can i use a quey? */   
    PA_SEND_QUEY,   /* Send me to the FIFO */
    PA_FREE_QUEY,   /* Quey free */
    PA_EX_GOOD,     /* Excange good */
    PA_RQ_GOOD,     /* Request good */
    PA_EOT,         /* End of trasmition */     
} ProtocolActions;

typedef struct {
    long int msgType;
    union {
        char msg[sizeof(int) * 4];
        struct {
            int id;
            ProtocolActions action;
            int goodId;
            int quantity;
        } data;
    } msg;
} PortMessage;

int sendMessage(int msgQueueId, int boatId, ProtocolActions action, int goodId, int quantity);
int reciveMessageById(int msgQueueId, int boatId, PortMessage* pMsg); 
int reciveMessage(int msgQueueId, PortMessage* pMsg); 