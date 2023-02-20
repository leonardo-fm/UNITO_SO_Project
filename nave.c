#define _GNU_SOURCES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    
#include <string.h>

#include <sys/shm.h>  
#include <sys/msg.h> 
#include <semaphore.h>
#include <fcntl.h>      
#include <errno.h>

#include <time.h>
#include <signal.h>

#include <math.h>

#include "lib/utilities.h"
#include "lib/config.h"
#include "lib/msgPortProtocol.h"

#include "lib/nave.h"

int *configArr;

int goodAnalyzerSharedMemoryId; 
int boatAnalyzerSharedMemoryId; 

int portSharedMemoryId; 
int currentMsgQueueId = -1;
int currentPort = -1;
int readingMsgQueue = -1;
int writingMsgQueue = -1;

Boat boat;
Goods *goodHold;

int simulationRunning = 1;

void handle_boat_simulation_signals(int signal) {

    switch (signal)
    {
        /* Start of the simulation */
        case SIGUSR1:
            break;

        /* Wait for the new day to come */
        case SIGUSR2:
            dumpData();
            waitForNewDay();
            newDay();
            break;

        /* Need to handle the signal for the waitForNewDay() */
        case SIGCONT:
            break;

        /* End of the simulation */
        case SIGSYS:
            dumpData();
            simulationRunning = 0;
            break;
        default:
            printf("Intercept a unhandled signal: %d\n", signal);
            break;
    }
}

void handle_boat_stopProcess() {
    printf("Stopping boat...\n");
    cleanup();
    exit(0);
}

/* argv[0]=id | argv[1]=configsh | argv[2]=portsh | argv[3]=ganalizersh | argv[4]=banalyzersh */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeEnvironment();
    initializeSingalsHandlers();

    if (initializeConfig(argv[1], argv[3], argv[4]) == -1) {
        printf("Initialization of boat config failed\n");
        safeExit(1);
    }

    if (initializeBoat(argv[0], argv[2]) == -1) {
        printf("Initialization of boat %s failed\n", argv[0]);
        safeExit(2);
    }

    if (work() == -1) {
        printf("Error during boat %d work\n", boat.id);
        safeExit(3);
    }

    if (cleanup() == -1) {
        printf("Boat cleanup failed\n");
        safeExit(4);
    }

    return 0;
}

int initializeSingalsHandlers() {

    struct sigaction signalAction;

    setpgid(getpid(), getppid());

    signal(SIGUSR1, handle_boat_simulation_signals);

    /* Use different method because i need to use the handler multiple times */
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_boat_simulation_signals;
    sigaction(SIGUSR2, &signalAction, NULL);

    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_boat_simulation_signals;
    sigaction(SIGCONT, &signalAction, NULL);

    signal(SIGSYS, handle_boat_simulation_signals);
    signal(SIGINT, handle_boat_stopProcess);

    return 0;
}

int initializeConfig(char *configShareMemoryIdString, char *goodAnalyzerShareMemoryIdString, char *boatAnalyzerShareMemoryIdString) {

    char *p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    
    goodAnalyzerSharedMemoryId = strtol(goodAnalyzerShareMemoryIdString, &p, 10);
    boatAnalyzerSharedMemoryId = strtol(boatAnalyzerShareMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        printf("The config key as failed to be conveted in boat\n");
        return -1;
    }

    return 0;
}

int initializeBoat(char *boatIdS, char *portShareMemoryIdS) {

    int i = 0;
    char *p;
    boat.id = strtol(boatIdS, &p, 10);
    boat.position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    boat.capacityInTon = configArr[S0_CAPACITY];
    boat.speed = configArr[SO_SPEED];
    boat.state = In_Sea_Empty;

    portSharedMemoryId = strtol(portShareMemoryIdS, &p, 10);

    /* Initialization of the hold */
    goodHold = malloc(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodHold == NULL) {
        printf("Error during malloc of boat hold\n");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods emptyGood;
        emptyGood.id = i;
        emptyGood.loadInTon = 0;
        emptyGood.state = In_The_Boat;

        goodHold[i] = emptyGood;
    }
    
    return 0;
}

int work() {

    /* wait for simulation to start */
    if (waitForStart() != 0) {
        printf("Error while waiting for start\n");
        return -1;
    }

    while (simulationRunning == 1)
    {   
        int tradeStatus;

        if (simulationRunning == 1) {
            if (gotoPort() == -1) {
                printf("Error while going to a port\n");
                return -1;
            }
        }

        if (simulationRunning == 1) {
            if (setupTrade(currentPort) == -1) {
                printf("Error during setup for the trade\n");
                return -1;
            }

            boat.state = In_Port_Exchange;

            tradeStatus = openTrade();
            if (tradeStatus == -1) {
                printf("Error during trade\n");
                return -1;
            } 

            if (haveIGoodsToSell() == 0) {
                boat.state = In_Sea;
            } else {
                boat.state = In_Sea_Empty;
            }
        }
    }

    return 0;
}

int waitForStart() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGUSR1);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int dumpData() {

    int i, goodReferenceId, boatReferenceId;
    goodDailyDump *goodArr;
    goodDailyDump gdd;

    boatDailyDump *boatArr;
    boatDailyDump bdd;

    /* Send goods data */
    goodReferenceId = boat.id * configArr[SO_MERCI];
    goodArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodArr == (void*) -1) {
        printf("Error during opening good analyzer share memory in boat\n");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {

        gdd.goodId = i;
        if (goodHold[i].state == Expired_In_The_Boat) {
            gdd.Good_Expired_In_The_Boat = goodHold[i].loadInTon;
            
            goodHold[i].loadInTon = 0;
            goodHold[i].state = Undefined;
        } else {
            gdd.Good_Expired_In_The_Boat = 0;
        }
        gdd.Good_In_The_Boat = goodHold[i].loadInTon;

        gdd.Good_Delivered = 0;
        gdd.Good_Expired_In_The_Port = 0;
        gdd.Good_In_The_Port = 0;

        memcpy(&goodArr[goodReferenceId + i], &gdd, sizeof(goodDailyDump)); 
    }

    if (shmdt(goodArr) == -1) {
        printf("The arr good detach failed in boat\n");
        return -1;
    }

    /* Send boat data */
    boatReferenceId = boat.id - configArr[SO_PORTI];
    boatArr = (boatDailyDump*) shmat(boatAnalyzerSharedMemoryId, NULL, 0);
    if (boatArr == (void*) -1) {
        printf("Error during opening boat analyzer share memory in boat\n");
        return -1;
    }

    bdd.id = boat.id;
    bdd.boatState = boat.state;
    boatArr[boatReferenceId] = bdd;

    if (shmdt(boatArr) == -1) {
        printf("The arr boat detach failed\n");
        return -1;
    }

    return 0;
}

int waitForNewDay() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGCONT);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int newDay() {

    int i;

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(goodHold[i].remaningDays > 0) {
            goodHold[i].remaningDays--;
            if (goodHold[i].remaningDays == 0) {
                goodHold[i].state = Expired_In_The_Boat;
            }
        }
    }

    return 0;
}

int gotoPort() {

    int newPortFound = 0;
    Port *portArr;
    double distance, kmPerDay;
    long waitTimeNs;
    int waitTimeS;

    portArr = (Port*) shmat(portSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        printf("Error during opening ports coordinate share memory\n");
        return -1;
    }

    while (newPortFound == 0)
    {
        int randomPort = getRandomValue(0, configArr[SO_PORTI] - 1);

        if (randomPort != currentPort) {
            newPortFound = 1;
            currentPort = randomPort;
        }
    }

    distance = sqrt(pow(portArr[currentPort].position.x - boat.position.x, 2) 
                    + pow(portArr[currentPort].position.y - boat.position.y, 2));
    
    if (shmdt(portArr) == -1) {
        printf("The arr port coordinates detach failed\n");
        return -1;
    }

    kmPerDay = distance / configArr[SO_SPEED];
    waitTimeNs = getNanoSeconds(kmPerDay);
    waitTimeS = getSeconds(kmPerDay);

    if (safeWait(waitTimeS, waitTimeNs) == -1) {
        printf("Error while waiting to go to in a port\n");
        return -1;
    }

    return 0;
}

int setupTrade(int portId) {

    Port *portArr;

    readingMsgQueue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    writingMsgQueue = msgget(IPC_PRIVATE, IPC_CREAT | 0600);

    if (currentMsgQueueId != -1) {
        printf("The old comunication with id %d was not closed properly\n", currentMsgQueueId);
        return -1;
    }
    
    portArr = (Port*) shmat(portSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        printf("Error during opening ports coordinate share memory\n");
        return -1;
    }

    currentMsgQueueId = portArr[portId].msgQueuId;
    if (sendMessage(currentMsgQueueId, PA_SETUP, writingMsgQueue, readingMsgQueue) == -1) {
        printf("Failed to send SETUP comunication\n");
        return -1;
    }

    if (shmdt(portArr) == -1) {
        printf("The arr port msg queue detach failed\n");
        return -1;
    }

    return 0;
}

/* Return 0 if the trade is a success, 1 if the port refuse to accept the boat, -1 for errors */
int openTrade() {

    int waitResponse;
    PortMessage response;

    if (sendMessage(writingMsgQueue, PA_ACCEPT, 0, 0) == -1) {
        printf("Failed to send ACCEPT comunication on channel %d\n", writingMsgQueue);
        return -1;
    }

    /* Wait for the response */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            printf("Error during waiting response from ACCEPT\n");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }
    
    if (response.msg.data.action == PA_N) {

        if (msgctl(readingMsgQueue, IPC_RMID, NULL) == -1) {
            printf("BOAT R The queue failed to be closed\n");
            return -1;
        }

        if (msgctl(writingMsgQueue, IPC_RMID, NULL) == -1) {
            printf("BOAT W The queue failed to be closed\n");
            return -1;
        }

        return 1;
    }
    
    if (trade() == -1) {
        printf("Error during trade\n");
        return -1;
    }

    return 0;
}

int trade() {

    if (haveIGoodsToSell() == 0 && simulationRunning == 1) {
        
        if (sellGoods() == -1) {
            printf("Error during selling goods\n");
            return -1;
        }
    }

    if (haveIGoodsToBuy() == 0 && simulationRunning == 1) {

        if (buyGoods() == -1) {
            printf("Error during buying goods\n");
            return -1;
        }
    }   

    if (closeTrade() == -1) {
        printf("Error during closing trade\n");
        return -1;
    }

    return 0;
}

int closeTrade() {

    int waitResponse;
    PortMessage response;

    if (sendMessage(writingMsgQueue, PA_EOT, 0, 0) == -1) {
        printf("Failed to send EOT comunication\n");
        return -1;
    }

    /* Default response to avoid random numbers if not initialized  */
    response.msg.data.action = PA_EOT;
    
    /* Wait for the response of end of transmission */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            printf("Error during waiting response from EOT\n");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }

    if (response.msg.data.action == PA_EOT) {

        currentMsgQueueId = -1;

        if (msgctl(readingMsgQueue, IPC_RMID, NULL) == -1) {
            printf("The reading queue failed to be closed\n");
            return -1;
        }
        readingMsgQueue = -1;

        if (msgctl(writingMsgQueue, IPC_RMID, NULL) == -1) {
            printf("The writing queue failed to be closed\n");
            return -1;
        }
        writingMsgQueue = -1;
    } else {
        printf("The boat while waiting for the EOT from the port recived something else\n");
    }

    return 0;
}

/* Return 0 if there are goods in the boat available for sale, otherwise -1 */
int haveIGoodsToSell() {
    
    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodHold[i].loadInTon > 0 ) {
            return 0;
        }    
    }

    return -1;
}

/* Return 0 if there are space available in the hold, otherwise -1 */
int haveIGoodsToBuy() {
    
    int i = 0;
    int totalNumOfTons = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        totalNumOfTons += goodHold[i].loadInTon;   
    }

    if (totalNumOfTons < boat.capacityInTon) {
        return 0;
    } else {
        return -1;
    }
}

int sellGoods() {

    int waitResponse, i, totalGoodSold;
    PortMessage response;

    char semaphoreKey[12];
    sem_t *semaphore;

    Goods *goodArr;

    /* Send request to sell */
    if (simulationRunning == 1) {
        if (sendMessage(writingMsgQueue, PA_SE_GOOD, 0, 0) == -1) {
            printf("Failed to send PA_SE_GOOD comunication\n");
            return -1;
        }
    }

    /* Default response to avoid random numbers if not initialized */
    response.msg.data.action = PA_N;

    /* Wait response */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            printf("Error during waiting response from PA_SE_GOOD\n");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }

    if (response.msg.data.action == PA_N) {
        return 0;
    }

    /* Get semaphore */
    if (sprintf(semaphoreKey, "%d", response.msg.data.data2) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Boat failed to found semaphore (SG) with key %s\n", semaphoreKey);
        return -1;
    }

    /* Get shared memory of the port */
    goodArr = (Goods*) shmat(response.msg.data.data1, NULL, 0);
    if (goodArr == (void*) -1) {
        printf("Error opening goods shared memory\n");
        return -1;
    }

    /* Sell all available goods */
    totalGoodSold = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodHold[i].loadInTon > 0 && goodHold[i].state != Expired_In_The_Boat) {
            
            int exchange = 0;
            double loadTonPerDay;
            long waitTimeNs;
            int waitTimeS;

            if (goodArr[i].loadInTon == 0) {
                continue;
            }
            
            sem_wait(semaphore);

            /* If x >= 0 OK, x < 0 not enought good to sell */
            if (goodArr[i].loadInTon - goodHold[i].loadInTon >= 0) {
                exchange = goodHold[i].loadInTon;
                goodArr[i].loadInTon -= goodHold[i].loadInTon;
                goodHold[i].loadInTon = 0;
            } else {
                exchange = goodHold[i].loadInTon - goodArr[i].loadInTon;
                goodArr[i].loadInTon = 0;
                goodHold[i].loadInTon -= exchange;
            }

            /* Set good state for boat */
            if (goodHold[i].loadInTon == 0) {
                goodHold[i].state = Undefined;
            }

            /* Set good state for port */
            if (goodArr[i].loadInTon > 0) {
                goodArr[i].state = In_The_Port;
            }

            sem_post(semaphore);

            loadTonPerDay = exchange / configArr[SO_LOADSPEED];
            waitTimeNs = getNanoSeconds(loadTonPerDay);
            waitTimeS = getSeconds(loadTonPerDay);

            if (safeWait(waitTimeS, waitTimeNs) == -1) {
                printf("Error while waiting to exchange\n");
                return -1;
            }

            totalGoodSold += exchange;
        }    
    }

    /* Send sell report to the port */
    if (sendMessage(writingMsgQueue, PA_SE_SUMMARY, totalGoodSold, 0) == -1) {
        printf("Failed to send PA_SE_SUMMARY comunication\n");
        return -1;
    }

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the good semaphore\n");
        return -1;
    }

    if (shmdt(goodArr) == -1) {
        printf("The arr sell detach failed\n");
        return -1;
    }

    return 0;
}

int buyGoods() {

    int waitResponse, i, totalGoodRequested;
    PortMessage response;
    
    char semaphoreKey[12];
    sem_t *semaphore;

    Goods *goodArr;
    
    /* Send request to buy */
    if (simulationRunning == 1) {
        if (sendMessage(writingMsgQueue, PA_RQ_GOOD, 0, 0) == -1) {
            printf("Failed to send PA_RQ_GOOD comunication\n");
            return -1;
        }
    }

    /* Default response to avoid random numbers if not initialized */
    response.msg.data.action = PA_N;

    /* Wait response */
    waitResponse = 1;
    while (waitResponse == 1) {
        
        int msgResponse = receiveMessage(readingMsgQueue, &response, 0, 0);
        if (msgResponse == -1) {
            printf("Error during waiting response from PA_RQ_GOOD\n");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }

    if (response.msg.data.action == PA_N) {
        return 0;
    }

    /* Get semaphore */
    if (sprintf(semaphoreKey, "%d", response.msg.data.data2) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Boat failed to found semaphore (RG) with key %s\n", semaphoreKey);
        return -1;
    }

    /* Get shared memory of the port */
    goodArr = (Goods*) shmat(response.msg.data.data1, NULL, 0);
    if (goodArr == (void*) -1) {
        printf("Error opening goods shared memory\n");
        return -1;
    }

    /* Buy some available goods */
    totalGoodRequested = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodArr[i].loadInTon > 0 && goodArr[i].state != Expired_In_The_Port) {
            
            int availableSpace, exchange;
            long waitTimeNs;

            sem_wait(semaphore);

            availableSpace = getSpaceAvailableInTheHold();
            exchange = 0;

            if (goodArr[i].loadInTon == 0) {
                goodArr[i].state = In_The_Boat;
            }

            /* If x >= 0 OK, x < 0 not enought good to buy */
            if (goodArr[i].loadInTon - availableSpace >= 0) {
                exchange = availableSpace;
                goodArr[i].loadInTon -= availableSpace;
                goodHold[i].loadInTon += availableSpace;
            } else {
                exchange = availableSpace - goodArr[i].loadInTon;
                goodArr[i].loadInTon = 0;
                goodHold[i].loadInTon += exchange;
            }

            /* Set good state for boat */
            if (goodHold[i].loadInTon > 0) {
                goodHold[i].state = In_The_Boat;
            }

            /* Set good state for port */
            if (goodArr[i].loadInTon == 0) {
                goodArr[i].state = Undefined;
            }

            sem_post(semaphore);

            waitTimeNs = getNanoSeconds(exchange / configArr[SO_LOADSPEED]);
            if (safeWait(0, waitTimeNs) == -1) {
                printf("Error while waiting to exchange\n");
                return -1;
            }

            totalGoodRequested += exchange;
            
            if (getSpaceAvailableInTheHold() == 0) {
                break;
            }
        }    
    }

    /* Send request report to the port */
    if (sendMessage(writingMsgQueue, PA_RQ_SUMMARY, totalGoodRequested, 0) == -1) {
        printf("Failed to send PA_RQ_SUMMARY comunication\n");
        return -1;
    }

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the good semaphore\n");
        return -1;
    }

    if (shmdt(goodArr) == -1) {
        printf("The arr buy detach failed\n");
        return -1;
    }
    
    return 0;
}

/* Return how many space have in the boat hold */
int getSpaceAvailableInTheHold() {

    int i = 0;
    int totalNumOfTons = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        totalNumOfTons += goodHold[i].loadInTon;   
    }

    return boat.capacityInTon - totalNumOfTons;
}

int cleanup() {
    printf("Boat clean\n");

    if (shmdt(configArr) == -1) {
        printf("The config detach failed\n");
        return -1;
    }

    return 0;
}

void safeExit(int exitNumber) {
    cleanup();
    printf("Program %s exit with error %d\n", __FILE__, exitNumber);
    kill(getppid(), SIGINT);
    exit(exitNumber);
}