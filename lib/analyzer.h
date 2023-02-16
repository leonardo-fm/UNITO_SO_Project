int initializeSingalsHandlers();
int initializeConfig(char *configShareMemoryIdString);
int initializeAnalyzer(char *goodAnalyzerShareMemoryIdString, 
    char *boatAnalyzerShareMemoryIdString, char *portAnalyzerShareMemoryIdString, char *wmsgq, char *rmsgq);
int createLogFile();
int waitForNewDay();
int work();
int checkDataDump();
int generateDailyHeader(FILE *filePointer);
int generateDailyGoodReport(FILE *filePointer);
int generateDailyBoatReport(FILE *filePointer);
int generateDailyPortReport(FILE *filePointer);
int cleanup();