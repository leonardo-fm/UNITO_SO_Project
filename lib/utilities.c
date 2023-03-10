#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "utilities.h"
#include "customMacro.h"

int simulationFinished = 0;
struct timespec remaningWaitingTime;
struct timespec addingTime;


/* Initialize some values for the program to work */
void initializeEnvironment() {

    int randomSeed = (rand() + (int)time(NULL)) % getpid();
    srand(randomSeed);
}

/* Generates coordinates given the dimensions of a plane. */
/* The function used to generate the random values has been taken from: */
/* https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c */
Coordinates getRandomCoordinates(int maxX, int maxY) {

    Coordinates coordinates;
    coordinates.x = (double) rand() / (RAND_MAX / maxX);
    coordinates.y = (double) rand() / (RAND_MAX / maxY);
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

void generateSubgroupSums(int *arr, int totalNumber, int subGroups) {

    int remaining, i;
    
    remaining = totalNumber;
    for (i = 0; i < subGroups - 1; i++) {
        
        int averageValue, randomAmmount;
        
        averageValue = remaining / (subGroups - i);
        if (averageValue == 0) {
            arr[i] = 0;
            continue;
        }

        randomAmmount = rand() % averageValue;
        remaining -= randomAmmount;
        
        arr[i] = randomAmmount;
    }

    arr[subGroups - 1] = remaining;
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

    struct timespec waitTime, remainingTime;
    int sleepStatus;

    waitTime.tv_sec = timeToSleepSec;
    waitTime.tv_nsec = timeToSleepNs;

    if (simulationFinished == 1) {
        return 0;
    }

    do {
        /* Reset errno error */
        errno = 0; 
        if (addingTime.tv_sec != 0 || addingTime.tv_nsec != 0) {
            waitTime.tv_sec += addingTime.tv_sec;
            waitTime.tv_nsec += addingTime.tv_nsec;
            addingTime.tv_sec = 0;
            addingTime.tv_nsec = 0;            
        }

        sleepStatus = nanosleep(&waitTime , &remainingTime);
        if (sleepStatus == 0) {
            break;
        }

        waitTime = remainingTime;
    } while (sleepStatus == -1 && errno == EINTR && simulationFinished == 0);
    
    if (errno != EINTR && errno != 0) {
        handleErrno("nanosleep()");
        return -1;
    }

    return 0;
}

int checkForWaiting() {
    
    double waitTimeS = remaningWaitingTime.tv_sec;
    double waitTimeNs = remaningWaitingTime.tv_nsec;

    remaningWaitingTime.tv_sec = 0;
    remaningWaitingTime.tv_nsec = 0;
    
    if (safeWait(waitTimeS, waitTimeNs) == -1) {
        handleError("Error while waiting the storm");
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