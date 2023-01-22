#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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