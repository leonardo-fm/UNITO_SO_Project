int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString, char *goodAnalyzerShareMemoryIdString, 
    char *portAnalyzerShareMemoryIdString, char *acknowledgeInitShareMemoryIdS, char *endGoodShareMemoryIdS,
    char *acknowledgeDumpShareMemoryIdS, char *goodMasterShareMemoryIdS);
int initializePort(char *portIdString, char *portShareMemoryIdS);
int initializePortStruct(char *portIdString, char *portShareMemoryIdS);
int initializeExchangeGoods();
int initializePortGoods();
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
int generateShareMemory(int sizeOfSegment);
sem_t *generateSemaphore(int semKey);
int cleanup();
void safeExit(int exitNumber);