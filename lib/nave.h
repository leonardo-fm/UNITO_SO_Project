int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString, char *goodAnalyzerShareMemoryIdString, char *boatAnalyzerShareMemoryIdString);
int initializeBoat(char *boatIdS, char *shareMemoryIdS);
int work();
int waitForStart();
int dumpData();
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