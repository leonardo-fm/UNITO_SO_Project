#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  

#include "lib/config.h"
#include "lib/utilities.h"

int main(int argx, char* argv[]) {
    
    char* p;
    int id = strtol(argv[0], &p, 10);
    int shareMemoryId = strtol(argv[1], &p, 10);
    Coordinates* arr = (Coordinates*) shmat(shareMemoryId, NULL, 0);
    Coordinates currentCoordinates = getRandomCoordinates(SO_LATO, SO_LATO);
    arr[id] = currentCoordinates;

    return 0;
}