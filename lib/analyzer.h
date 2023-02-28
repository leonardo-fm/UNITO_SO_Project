void initializeSingalsMask();
int initializeSingalsHandlers();
int initializeConfig(char *configSharedMemoryIdString);
int initializeAnalyzer(char *goodAnalyzerSharedMemoryIdString, 
    char *boatAnalyzerSharedMemoryIdString, char *portAnalyzerSharedMemoryIdString, 
    char *wmsgq, char *rmsgq, char *endGoodSharedMemoryIdString, char *acknowledgeSharedMemoryIdString,
    char *acknowledgeDumpSharedMemoryIdString);
int createLogFile();
int waitForNewDay();
int work();
int checkDataDump();
int generateDailyDump();
int generateDailyHeader();
int generateDailyGoodReport();
int generateDailyBoatReport();
int generateDailyPortReport();
int generateEndDump();
int generateEndHeader();
int generateEndGoodReport();
int generateEndPortStat();
void setAcknowledge();
int cleanup();
void safeExit(int exitNumber);