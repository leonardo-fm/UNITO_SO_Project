void initializeSingalsMask();
int initializeSingalsHandlers();
int initializeConfig(char *configSharedMemoryIdString);
int initializeWeather(char *boatSharedMemoryIdS, char *portSharedMemoryIdS, char *acknowledgeDumpSharedMemoryIdS);
int work();
int activateSwell();
int activateStorm();
int activateMalestorm();
int insertNewWeatherStatus(int pid, int remainingHours, int signalId);
int checkWeatherStatus();
void setAcknowledge();
int cleanup();
void safeExit(int exitNumber);