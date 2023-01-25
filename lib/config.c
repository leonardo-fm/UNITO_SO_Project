#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/shm.h>

#include "models.h"

/* Load the configuration file (config.txt) from the root directory of the project. */
/* Return 0 if the configuration has been loaded succesfully, -1 if some errors occurred. */
int loadConfig(int configShareMemoryId) {
    
    FILE* filePointer;
    filePointer = fopen("config.txt", "r");
    if (filePointer == NULL) {
        printf("loadingConfig | The file pointer of the config file is null\n");
        return -1;
    }

    int* arr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        return -1;
    }

    int i = 0;

    char fileLine[150];
    while (!feof(filePointer)) {
        if (fgets(fileLine, 150, filePointer) == NULL) {
            printf("loadingConfig | The fgets() result is null\n");
            return -1;
        }
        char* configValue;
        char* p;
        /* Adding the char to the pointer to remove the = */
        configValue = memchr(fileLine, '=', strlen(fileLine)) + sizeof(char);
        if (configValue == NULL) {
            printf("loadingConfig | Not found config value\n");
            return -1;
        }

        arr[i] = strtol(configValue, &p, 10);
        i++;
    }

    fclose(filePointer);
    
    return 0;
}