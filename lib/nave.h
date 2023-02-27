int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString, char *goodAnalyzerShareMemoryIdString, 
    char *boatAnalyzerShareMemoryIdString, char *acknowledgeInitShareMemoryIdS, char *endGoodShareMemoryIdS,
    char *acknowledgeDumpShareMemoryIdS);
int initializeBoat(char *boatIdS, char *portShareMemoryIdS);
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
int cleanup();
void safeExit(int exitNumber);