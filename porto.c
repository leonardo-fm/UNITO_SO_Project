#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     

int main(int argx, char* argv[]) {
    
    int id = strtol(argv[0], NULL, 10);
    printf("Given id: %d\n", id);

    return 0;
}