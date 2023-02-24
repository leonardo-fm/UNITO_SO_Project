int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString);
int initializeAnalyzer(char *goodAnalyzerShareMemoryIdString, 
    char *boatAnalyzerShareMemoryIdString, char *portAnalyzerShareMemoryIdString, 
    char *wmsgq, char *rmsgq, char *endGoodShareMemoryIdString, char *acknowledgeShareMemoryIdString);
int createLogFile();
int waitForNewDay();
int waitForStart();
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