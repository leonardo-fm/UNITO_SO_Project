#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#include "models.h"
#include "customMacro.h"

/* Initialize some values for the program to work */
void initializeEnvironment() {

    srand(getpid());
}

/* Generates coordinates given the dimensions of a plane. */
/* The function used to generate the random values has been taken from: */
/* https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c */
Coordinates getRandomCoordinates(double maxX, double maxY) {

    Coordinates coordinates;
    coordinates.x = rand() / (RAND_MAX / maxX);
    coordinates.y = rand() / (RAND_MAX / maxY);
    return coordinates;
}

/* Generates corner coordinates given the dimensions of a plane. */
Coordinates getCornerCoordinates(int maxX, int maxY, int num) {

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

/* Return a random value between min and max inclusive */
int getRandomValue(int min, int max) {

    int range = max - min + 1;
    int randomValue = rand() % range + min;

    return randomValue;
}

/* Generate by ChatGPT */
void generateSubgroupSums(int *arr, int totalNumber, int subgroups) {

    int remaining, i, j, temp;
    
    remaining = totalNumber;
    for (i = 0; i < subgroups - 1; i++) {
        int rand_num = rand() % (remaining - (subgroups - i - 1)) + 1;
        arr[i] = rand_num;
        remaining -= rand_num;
    }

    arr[subgroups - 1] = remaining;

    /* Shuffle the array to ensure randomness */
    for (i = subgroups - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

int getSeconds(double timeInSeconds) {
    double d;
    modf(timeInSeconds, &d);
    return d;
}

long getNanoSeconds(double timeInSeconds) {
    double d, f;
    f = modf(timeInSeconds, &d);
    return f * pow(10, 9);
}

int safeWait(int timeToSleepSec, long timeToSleepNs) {

    struct timespec ts1, ts2;
    int sleepStatus;

    ts1.tv_sec = timeToSleepSec;
    ts1.tv_nsec = timeToSleepNs;

    do {
        /* Reset errno error */
        errno = 0; 

        sleepStatus = nanosleep(&ts1 , &ts2);
        if (sleepStatus == 0) {
            break;
        }

        ts1 = ts2;
    } while (sleepStatus == -1 && errno == EINTR);
    
    if (errno != EINTR && errno != 0) {
        handleErrno("nanosleep()");
        return -1;
    }

    return 0;
}

int waitForSignal(int signal) {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, signal);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}