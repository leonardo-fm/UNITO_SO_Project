#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     

#include "lib/utilities.h"
#include "lib/config.h"

int main(int argx, char* argv[]) {

    Coordinates c = getRandomCoordinates(SO_LATO, SO_LATO);
    printf("Hello from process %d\n", getpid());
    return 0;
}