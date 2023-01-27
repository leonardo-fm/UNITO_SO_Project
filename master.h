int initializeSingalsHandlers();
int work();
int generateShareMemory(int sizeOfSegment);
int generateSubProcesses(int nOfProcess, char* execFilePath, int configShareMemoryId, int portShareMemoryId, int goodShareMemoryId);
int initializeGoods(int goodShareMemoryId);
int generateGoods(int goodShareMemoryId);
int generateSemaphore();
int cleanup(int pointerIdArr[], int size);