#include <stdio.h>
#include <signal.h>

int *configArr;
int *goodAnalyzerSharedMemoryPointer;
int *boatAnalyzerSharedMemoryPointer; 
int *portAnalyzerSharedMemoryPointer;

char *logPath = "/out";

void handle_analyzer_stopProcess() {
    
    cleanup();
    exit(0);
}

int main(int argx, char *argv[]) {
    
    (void) argx;
    initializeSingalsHandlers();

    if (initializeConfig(argv[0], argv[4]) == -1) {
        printf("Initialization of analyzer config failed\n");
        exit(1);
    }

    if (initializeAnalyzer() == -1) {
        printf("Initialization of analyzer failed\n");
        exit(2);
    }

    if (work() == -1) {
        printf("Error during analyzer work\n");
        exit(3);
    }

    if (cleanup() == -1) {
        printf("Analyzer cleanup failed\n");
        exit(4);
    }

    return 0;
}

int initializeSingalsHandlers() {

    struct sigaction signalAction;

    setpgid(getpid(), getppid());

    signal(SIGINT, handle_analyzer_stopProcess);

    return 0;
}

int initializeConfig(char *configShareMemoryIdString, char *analyzerShareMemoryIdString) {
/*
    char *p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    int analyzerShareMemoryId = strtol(analyzerShareMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        printf("The config key as failed to be conveted in analyzer\n");
        return -1;
    }

    analyzerSharedMemoryPointer = (int*) shmat(analyzerShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        printf("The analyzer key as failed to be conveted in analyzer\n");
        return -1;
    }
*/
    return 0;
}

int initializeAnalyzer() {

    

    return 0;
}

int work() {

    return 0;
}

int cleanup() {
/*
    if (shmctl(goodStockShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }
    
    if (shmctl(goodStockShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }

    if (shmctl(goodStockShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }
*/
    return 0;
}