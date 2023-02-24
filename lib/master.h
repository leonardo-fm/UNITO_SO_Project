int initializeSingalsHandlers();
int acknowledgeChildrenStatus();
int work();
int generateShareMemory(int sizeOfSegment);
int generateSemaphore(int semKey);
int generateSubProcesses(int nOfProcess, char *execFilePath, int includeProceduralId, int *arguments, int argSize);
int initializeGoods(int goodShareMemoryId);
int generateGoods(int goodShareMemoryId);
int cleanup();
void safeExit(int exitNumber);