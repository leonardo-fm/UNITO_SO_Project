int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString, char *goodAnalyzerShareMemoryIdString, 
    char *boatAnalyzerShareMemoryIdString, char *acknowledgeInitShareMemoryIdS, char *endGoodShareMemoryIdS,
    char *acknowledgeDumpShareMemoryIdS);
int initializeBoat(char *boatIdS, char *portShareMemoryIdS);
int work();
int waitForStart();
int dumpData();
int waitForDumpData();
int waitForNewDay();
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
int cleanup();
void safeExit(int exitNumber);