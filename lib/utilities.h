#include "models.h"

extern int simulationFinished;
extern struct timespec remaningWaitingTime;
extern struct timespec addingTime;

void initializeEnvironment();

Coordinates getRandomCoordinates(int maxX, int maxY);
Coordinates getCornerCoordinates(int maxX, int maxY, int num);

int getRandomValue(int min, int max);
void generateSubgroupSums(int *arr, int totalNumber, int subGroups);

int getSeconds(double timeInSeconds);
long getNanoSeconds(double timeInSeconds);
int safeWait(int timeToSleepSec, long timeToSleepNs);
int checkForWaiting();

int waitForSignal(int signal);