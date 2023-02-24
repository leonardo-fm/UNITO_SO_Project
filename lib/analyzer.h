int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString);
int initializeAnalyzer(char *goodAnalyzerShareMemoryIdString, 
    char *boatAnalyzerShareMemoryIdString, char *portAnalyzerShareMemoryIdString, 
    char *wmsgq, char *rmsgq, char *endGoodShareMemoryIdString);
int createLogFile();
int waitForNewDay();
int waitForStart();
int work();
int checkDataDump();
int generateDailyDump(FILE *filePointer);
int generateDailyHeader(FILE *filePointer);
int generateDailyGoodReport(FILE *filePointer);
int generateDailyBoatReport(FILE *filePointer);
int generateDailyPortReport(FILE *filePointer);
int generateEndDump(FILE *filePointer);
int generateEndHeader(FILE *filePointer);
int generateEndGoodReport(FILE *filePointer);
int generateEndPortStat(FILE *filePointer);
int cleanup();
void safeExit(int exitNumber);