int initializeSingalsHandlers();
int work();
int generateShareMemory(int sizeOfSegment);
int generateSubProcesses(int nOfProcess, char *execFilePath, int includeProceduralId, int *arguments, int argSize);
int initializeGoods(int goodShareMemoryId);
int generateGoods(int goodShareMemoryId);
int generateSemaphore();
int cleanup();