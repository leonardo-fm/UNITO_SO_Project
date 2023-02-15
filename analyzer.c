#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>

#include <linux/limits.h>

int *configArr;

int goodAnalyzerSharedMemoryId;
int boatAnalyzerSharedMemoryId; 
int portAnalyzerSharedMemoryId;

char *logPath;

int currentDay = 0;
int simulationRunning = 1;

void handle_analyzer_newDay() {
    
}

void handle_analyzer_endSimulation() {
    simulationRunning = 0;
}


void handle_analyzer_stopProcess() {
    
    cleanup();
    exit(0);
}

/* argv[0]=id | argv[1]=ganalizersh | argv[2]=banalyzersh | argv[3]=panalyzersh */
int main(int argx, char *argv[]) {
    
    (void) argx;
    initializeSingalsHandlers();

    if (initializeConfig(argv[0]) == -1) {
        printf("Initialization of analyzer config failed\n");
        exit(1);
    }

    if (initializeAnalyzer(argv[1], argv[2], argv[3]) == -1) {
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

    signal(SIGUSR2, handle_analyzer_newDay);
    signal(SIGSYS, handle_analyzer_endSimulation);
    signal(SIGINT, handle_analyzer_stopProcess);

    return 0;
}

int initializeConfig(char *configShareMemoryIdString) {

    char *p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        printf("The config key as failed to be conveted in analyzer\n");
        return -1;
    }

    return 0;
}

int initializeAnalyzer(char *goodAnalyzerShareMemoryIdString, char *boatAnalyzerShareMemoryIdString, char *portAnalyzerShareMemoryIdString) {

    char *p;

    int goodAnalyzerSharedMemoryId = strtol(goodAnalyzerShareMemoryIdString, &p, 10);
    int boatAnalyzerSharedMemoryId = strtol(boatAnalyzerShareMemoryIdString, &p, 10);
    int portAnalyzerSharedMemoryId = strtol(portAnalyzerShareMemoryIdString, &p, 10);

    createLogFile();

    return 0;
}

int createLogFile() {

    char currentWorkingDirectory[PATH_MAX];
    int fileDescriptor;

    if (getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)) == NULL) {
        printf("Error retreavin working directory\n");
        return -1;
    }

    fprintf(logPath, "%s/out/dump.txt", currentWorkingDirectory);

    fileDescriptor = fopen(logPath, "w");
    if (fileDescriptor == NULL) {
        printf("Error createing file\n");
        return -1;
    }

    if (fclose(fileDescriptor) != 0) {
        printf("Error closing file\n");
        return -1;
    }

    return 0;
}

int waitForNewDay() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGCONT);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int work() {

    while (simulationRunning == 1)
    {
        waitForNewDay();

        int fileDescriptor = fopen(logPath, "a");
        if (fileDescriptor == NULL) {
            printf("Error opening file\n");
            return -1;
        }

        if (generateDailyHeader(fileDescriptor) == -1) {
            printf("Error while writing header\n");
            return -1;
        }

        if (generateDailyGoodReport(fileDescriptor) == -1) {
            printf("Error while writing good report\n");
            return -1;
        }

        if (generateDailyBoatReport(fileDescriptor) == -1) {
            printf("Error while writing boat report\n");
            return -1;
        }

        if (generateDailyPortReport(fileDescriptor) == -1) {
            printf("Error while writing port report\n");
            return -1;
        }

        currentDay++;

        kill(getppid(), SIGALRM);    
    }
    
    return 0;
}

int generateDailyHeader(int fileDescriptor) {
    fprintf(fileDescriptor, "\t\t=====================\t\t\\nDay: %d\n", currentDay);
}

int generateDailyGoodReport(int fileDescriptor) {



    fprintf(fileDescriptor, "\t\t=====================\t\t\\nDay: %d\n", currentDay);
}

int cleanup() {

    if (shmctl(goodAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        printf("The good shared memory failed to be closed in analyzer\n");
        return -1;
    }
    
    if (shmctl(boatAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        printf("The boat shared memory failed to be closed in analyzer\n");
        return -1;
    }

    if (shmctl(portAnalyzerSharedMemoryId, IPC_RMID, NULL) == -1) {
        printf("The port shared memory failed to be closed in analyzer\n");
        return -1;
    }

    return 0;
}