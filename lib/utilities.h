#include "customMacro.h"
#include "models.h"

void initializeEnvironment();

Coordinates getRandomCoordinates(double maxX, double maxY);
Coordinates getCornerCoordinates(double maxX, double maxY, int num);

int getRandomValue(int min, int max);
void generateSubgroupSums(int *arr, int totalNumber, int subgroups);

int getSeconds(double timeInSeconds);
long getNanoSeconds(double timeInSeconds);
int safeWait(int timeToSleepSec, long timeToSleepNs);

int waitForSignal(int signal);