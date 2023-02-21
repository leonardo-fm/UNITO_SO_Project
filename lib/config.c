#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/shm.h>

#include "models.h"
#include "customMacro.h"
#include "config.h"

/* Load the configuration file (config.txt) from the root directory of the project. */
/* Return 0 if the configuration has been loaded succesfully, -1 if some errors occurred. */
int loadConfig(int configShareMemoryId) {
    
    int *arrConfig;
    char fileLine[150];
    int i = 0;

    FILE *filePointer;
    filePointer = fopen("config.txt", "r");
    if (filePointer == NULL) {
        handleError("The file pointer of the config file is null");
        return -1;
    }

    arrConfig = (int*) shmat(configShareMemoryId, NULL, 0);
    if (arrConfig == (void*) -1) {
        return -1;
    }

    while (!feof(filePointer)) {
        char *configValue;
        char *p;

        if (fgets(fileLine, 150, filePointer) == NULL) {
            handleErrno("fgets()");
            return -1;
        }

        configValue = memchr(fileLine, '=', strlen(fileLine));
        /* Adding the char to the pointer to remove the '=' */
        configValue += sizeof(char);
        if (configValue == NULL) {
            handleError("Not found config value");
            return -1;
        }

        arrConfig[i] = strtol(configValue, &p, 10);
        i++;
    }

    if (checkConfigValues(arrConfig) == -1) {
        handleError("Check config failed");
        return -1;
    }

    fclose(filePointer);
    
    return 0;
}

int checkConfigValues(int *arrConfig) {
    
    if(arrConfig[SO_NAVI] < 1) {
        handleError("SO_NAVI must be >= 1");
        return -1;
    }   

    if(arrConfig[SO_PORTI] < 4) {
        handleError("SO_PORTI must be >= 4");
        return -1;
    }

    return 0;
}