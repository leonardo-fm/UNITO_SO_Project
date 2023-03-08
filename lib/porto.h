void initializeSingalsMask();
int initializeSingalsHandlers();
int initializeConfig(char *configSharedMemoryIdString, char *goodAnalyzerSharedMemoryIdString, 
    char *portAnalyzerSharedMemoryIdString, char *acknowledgeInitSharedMemoryIdS, char *endGoodSharedMemoryIdS,
    char *acknowledgeDumpSharedMemoryIdS, char *goodMasterSharedMemoryIdS);
int initializePort(char *portIdString, char *portSharedMemoryIdS);
int initializePortStruct(char *portIdString, char *portSharedMemoryIdS);
int initializeExchangeGoods();
int getPortGoods();
int handleSwell();
int waitForSwell();
int work();
int dumpData();
int newDay();
int freePendingMsgs();
int handlePA_ACCEPT(int queueId);
int handlePA_SE_GOOD(int queueId);
int handlePA_SE_SUMMARY(int goodId, int exchangeQuantity);
int handlePA_RQ_GOOD(int queueId);
int handlePA_RQ_SUMMARY(int goodId, int exchangeQuantity);
int handlePA_EOT();
int generateSharedMemory(int sizeOfSegment);
sem_t *generateSemaphore(int semKey);
void setAcknowledge();
int cleanup();
void safeExit(int exitNumber);