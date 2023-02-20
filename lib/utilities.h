#include "models.h"

#define debug(S) (printf("%s line: %d, message: %s\n", __FILE__, __LINE__, S))

void initializeEnvironment();

Coordinates getRandomCoordinates(double maxX, double maxY);
Coordinates getCornerCoordinates(double maxX, double maxY, int num);

int getRandomValue(int min, int max);

int getSeconds(double timeInSeconds);
long getNanoSeconds(double timeInSeconds);
int safeWait(int timeToSleepSec, long timeToSleepNs);