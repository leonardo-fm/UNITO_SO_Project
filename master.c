#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     

#include "lib/config.h"

int main(int argx, char* argv[]) {
    if (loadConfig() == -1) {
        return 1;
    }

    return 0;
}