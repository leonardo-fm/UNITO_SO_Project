#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "models.h"

/* Generates coordinates given the dimensions of a plane. */
/* The function used to generate the random values has been taken from: */
/* https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c */
Coordinates getRandomCoordinates(double maxX, double maxY) {

    Coordinates coordinates;
    coordinates.x = (double) rand() / (double) (RAND_MAX / maxX);
    coordinates.y = (double) rand() / (double) (RAND_MAX / maxY);
    return coordinates;
}

/* Generates corner coordinates given the dimensions of a plane. */
Coordinates getCornerCoordinates(double maxX, double maxY, int num) {

    Coordinates coordinates;
    switch (num) {
        case 0:
            coordinates.x = 0;
            coordinates.y = 0;
            break;
        case 1:
            coordinates.x = maxX;
            coordinates.y = 0;
            break;
        case 2:
            coordinates.x = 0;
            coordinates.y = maxY;
            break;
        case 3:
            coordinates.x = maxX;
            coordinates.y = maxY;
            break;
        default:
            coordinates.x = -1;
            coordinates.y = -1;
            break;
    }
    
    return coordinates;
}

/* return a random value between min and max inclusive */
int getRandomValue(int min, int max) {

    return (rand() / (RAND_MAX / (max - (min - 1)))) + min;
}