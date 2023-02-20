#include "models.h"

void initializeEnvironment();

Coordinates getRandomCoordinates(double maxX, double maxY);
Coordinates getCornerCoordinates(double maxX, double maxY, int num);

int getRandomValue(int min, int max);

int getSeconds(double timeInSeconds);
long getNanoSeconds(double timeInSeconds);
int safeWait(int timeToSleepSec, long timeToSleepNs);