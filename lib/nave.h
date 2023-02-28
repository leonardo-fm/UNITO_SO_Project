void initializeSingalsMask();
int initializeSingalsHandlers();
int initializeConfig(char *configSharedMemoryIdString, char *goodAnalyzerSharedMemoryIdString, 
    char *boatAnalyzerSharedMemoryIdString, char *acknowledgeInitSharedMemoryIdS, char *endGoodSharedMemoryIdS,
    char *acknowledgeDumpSharedMemoryIdS);
int initializeBoat(char *boatIdS, char *portSharedMemoryIdS, char *boatSharedMemoryIdS);
int handleStorm();
void handleMalestorm();
int work();
int dumpData();
int newDay();
int gotoPort();
int setupTrade(int portId);
int openTrade();
int trade();
int closeTrade();
int haveIGoodsToSell();
int haveIGoodsToBuy();
int sellGoods();
int buyGoods();
int getSpaceAvailableInTheHold();
void setAcknowledge();
int cleanup();
void safeExit(int exitNumber);