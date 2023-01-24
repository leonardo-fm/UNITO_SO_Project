#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <signal.h>  
#include <errno.h>

#include "lib/config.h"
#include "lib/utilities.h"

void cleanUp (int num) {

    cleanEnvironment();
}

int main(int argx, char* argv[]) {

    signal(SIGINT, cleanUp);
    signal(SIGTERM, cleanUp);

    initializeEnvironment();
    if (loadConfig() == -1) {
        return 1;
    }

    int shareMemoryId = getSharedMemory(sizeof(Coordinates) * SO_PORTI);
    char shareMemoryIdString[12];
    if (sprintf(shareMemoryIdString, "%d", shareMemoryId) == -1) {
        printf("Error during conversion of the id of shared memory to a string");
        return 2;
    }

    if (GenerateSubProcesses(SO_PORTI, "./bin/porto", shareMemoryIdString) == -1) {
        return 3;
    }

    if (GenerateSubProcesses(SO_NAVI, "./bin/nave", shareMemoryIdString) == -1) {
        return 3;
    }

    return 0;
}

/* Generate processes by forking master and using execve */
/* Return 0 if the processes has been loaded succesfully, -1 if some errors occurred. */
int GenerateSubProcesses(int nOfProcess, char* execFilePath, char shareMemoryId[]) {

    if(nOfProcess <= 0) {
        return 0;
    }

    int i = 0;
    while (i++ < nOfProcess)
    {
        int id = fork();
        if (id == -1) {
            printf("Error during fork of the file %s\n", execFilePath);
            return -1;
        }
        if (id == 0) {  
            char idS[12];
            /* i - 1 because i need to have the ids to start from 0 to n */
            if (sprintf(idS, "%d", (i - 1)) == -1) {
                printf("Error during conversion of the id to a string");
                return -1;
            }

            char* arr[] = { idS, shareMemoryId, NULL };

            if (execve(execFilePath, arr, NULL) == -1) {
                printf("Error during innesting of the file %s\n", execFilePath);
                return -1;
            }
        } 
    }

    return 0;
}