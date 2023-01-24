#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/shm.h>

#include "utilities.h"

/* Generates coordinates given the dimensions of a plane. */
/* The function used to generate the random values has been taken from: */
/* https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c */
Coordinates getRandomCoordinates(double maxX, double maxY) {
    Coordinates coordinates;
    coordinates.x = (double) rand() / (double) (RAND_MAX / maxX);
    coordinates.y = (double) rand() / (double) (RAND_MAX / maxY);
    return coordinates;
}

/* The function used to generate a share memory of size sizeOfsm. */
/* Return the id of the sm >= 0 if the creation was a success, -1 if some errors occurred. */
int getSharedMemory(int sizeOfsm) {
    int segmentId = shmget(IPC_PRIVATE, sizeOfsm, 0600);
    return segmentId;
}