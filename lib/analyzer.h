int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString);
int initializeAnalyzer(char *goodAnalyzerShareMemoryIdString, 
    char *boatAnalyzerShareMemoryIdString, char *portAnalyzerShareMemoryIdString, 
    char *wmsgq, char *rmsgq, char *endGoodShareMemoryIdString, char *acknowledgeShareMemoryIdString, 
    char *acknowledgeDumpShareMemoryIdString);
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
int cleanup();
void safeExit(int exitNumber);