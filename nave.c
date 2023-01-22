#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     

#include "lib/utilities.h"
#include "lib/config.h"

int main(int argx, char* argv[]) {
    initializeEnvironment();
    int i = 0;
    for (i = 0; i < 10; i++) {
        Coordinates c = getRandomCoordinates(100.0, 100.0);
        printf("x: %f y: %f\n", c.x, c.y);
    }
    
    return 0;
}