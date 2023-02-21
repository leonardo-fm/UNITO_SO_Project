int initializeSingalsHandlers();
int waitForInitializationOfChildren();
int work();
int generateShareMemory(int sizeOfSegment);
int generateSubProcesses(int nOfProcess, char *execFilePath, int includeProceduralId, int *arguments, int argSize);
int initializeGoods(int goodShareMemoryId);
int generateGoods(int goodShareMemoryId);
int cleanup();
void safeExit(int exitNumber);