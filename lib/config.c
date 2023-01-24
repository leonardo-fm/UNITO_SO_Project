#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <string.h>
#include <errno.h>

int SO_NAVI = 0;
int SO_PORTI = 0;
int SO_MERCI = 0;
int SO_SIZE = 0;
int SO_MIN_VITA = 0;
int SO_MAX_VITA = 0;
double SO_LATO = 0;
int SO_SPEED = 0;
int S0_CAPACITY = 0;
int SO_BANCHINE = 0;
int SO_FILL = 0;
int SO_LOADSPEED = 0;
int SO_DAYS = 0; 

/* Initialize some values for the program to work */
void initializeEnvironment() {

    srand(time(NULL));
}

/* Used to clean after the program has crashed or finished */
void cleanEnvironment() {
    
    printf("Running cleaning process\n");
    system("bash killIPCS.sh");
}

/* Load the configuration file (config.txt) from the root directory of the project. */
/* Return 0 if the configuration has been loaded succesfully, -1 if some errors occurred. */
int loadConfig() {
    
    FILE* filePointer;
    filePointer = fopen("config.txt", "r");
    if (filePointer == NULL) {
        printf("loadingConfig | The file pointer of the config file is null\n");
        return -1;
    }

    long configValues[13];
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
        configValues[i++] = strtol(configValue, &p, 10);
    }

    fclose(filePointer);
    
    SO_NAVI = configValues[0];
    SO_PORTI = configValues[1];
    SO_MERCI = configValues[2];
    SO_SIZE = configValues[3];
    SO_MIN_VITA = configValues[4];
    SO_MAX_VITA = configValues[5];
    SO_LATO = configValues[6];
    SO_SPEED = configValues[7];
    S0_CAPACITY = configValues[8];
    SO_BANCHINE = configValues[9];
    SO_FILL = configValues[10];
    SO_LOADSPEED = configValues[11];
    SO_DAYS = configValues[12]; 
    
    return 0;
}