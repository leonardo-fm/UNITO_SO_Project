#define _GNU_SOURCES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    

#include <sys/shm.h>  
#include <sys/msg.h> 
#include <semaphore.h>
#include <fcntl.h>      

#include <time.h>
#include <signal.h>

#include <math.h>

#include "lib/utilities.h"
#include "lib/config.h"
#include "lib/msgPortProtocol.h"

#include "nave.h"

int NUM_OF_SETTINGS = 13;
int* configArr;

int portSharedMemoryPointer; 
int currentMsgQueueId = -1;
int currentPort = -1;

Boat boat;
Goods* goodHold;

int startSimulation = 0;
int simulationRunning = 1;

void handle_boat_start(int num) {

    startSimulation = 1;
}

void handle_boat_newDay() {

    newDay();
}

void handle_boat_stopSimulation() {

    simulationRunning = 0;
}


int main(int argx, char* argv[]) {

    (void) argx;
    initializeEnvironment();
    initializeSingalsHandlers();

    if (initializeConfig(argv[0]) == -1) {
        printf("Initialization of boat config failed\n");
        exit(1);
    }

    if (initializeBoat(argv[1], argv[2]) == -1) {
        printf("Initialization of boat %s failed\n", argv[0]);
        exit(2);
    }

    if (work() == -1) {
        printf("Error during boat %d work\n", boat.id);
        exit(3);
    }

    if (cleanup() == -1) {
        printf("Cleanup failed\n");
        exit(4);
    }

    return 0;
}

int initializeSingalsHandlers() {

    setpgid(getpid(), getppid());
    printf("listening on id: %d\n", getppid());

    struct sigaction sa1;
    sa1.sa_flags = SA_RESTART;
    sa1.sa_handler = &handle_boat_start;
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2;
    sa2.sa_flags = SA_RESTART;
    sa2.sa_handler = &handle_boat_newDay;
    sigaction(SIGUSR2, &sa2, NULL);

    struct sigaction sa3;
    sa3.sa_flags = SA_RESTART;
    sa3.sa_handler = &handle_boat_stopSimulation;
    sigaction(SIGTERM, &sa3, NULL);

    return 0;
}

int initializeConfig(char* configShareMemoryIdString) {

    char* p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        return -1;
    }

    return 0;
}

int initializeBoat(char* boatIdS, char* shareMemoryIdS) {

    char* p;
    boat.id = strtol(boatIdS, &p, 10);
    boat.position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    boat.capacityInTon = configArr[S0_CAPACITY];
    boat.speed = configArr[SO_SPEED];
    boat.state = In_Sea;

    portSharedMemoryPointer = strtol(shareMemoryIdS, &p, 10);

    /* Initialization of the hold */
    goodHold = malloc(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodHold == NULL) {
        printf("Error during malloc of boat hold\n");
        return -1;
    }

    int i = 0;
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
    while (startSimulation == 0) { };

    printf("AAAAAAAAAAAAAAAAAAA\n");
    while (simulationRunning == 1)
    {
        if (gotoPort() == -1) {
            printf("Error while going to a port\n");
            return -1;
        }

        int tradeStatus = openTrade(currentPort);
        if (tradeStatus == -1) {
            printf("Error during trade\n");
            return -1;
        }
    }

    return 0;
}

int newDay() {

    printf("Recived boat new day\n"); 

    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(goodHold[i].remaningDays > 0) {
            goodHold[i].remaningDays--;
            if (goodHold[i].remaningDays == 0) {
                goodHold[i].state = Expired_In_The_Boat;
            }
        }
    }

    /* TODO Dealy dump */

    return 0;
}

int gotoPort() {

    int newPortFound = 0;

    Coordinates* arr = (Coordinates*) shmat(portSharedMemoryPointer, NULL, 0);
    if (arr == (void*) -1) {
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

    double num = (double)(((arr[currentPort].x - boat.position.x) * (arr[currentPort].x - boat.position.x))) 
        + (double)(((arr[currentPort].y - boat.position.y) * (arr[currentPort].y - boat.position.y)));
    double distance = sqrt(num);

    if (waitExchange(distance / configArr[SO_SPEED]) == -1) {
        printf("Error while waiting to go to in a port\n");
        return -1;
    }

    return 0;
}

/* Return 0 if the trade is a success, 1 if the port refuse to accept the boat, -1 for errors */
int openTrade(int portId) {

    if (currentMsgQueueId != -1) {
        printf("The old comunication with id %d was not closed properly\n", currentMsgQueueId);
        return -1;
    }

    currentMsgQueueId = msgget((key_t) portId, IPC_CREAT | 0600);

    if (sendMessage(currentMsgQueueId, boat.id, PA_ACCEPT, 0, 0) == -1) {
        printf("Failed to send ACCEPT comunication\n");
        return -1;
    }

    int waitResponse = 1;
    PortMessage response;
    while (waitResponse == 1) {
        
        int msgResponse = reciveMessageById(currentMsgQueueId, boat.id, &response);
        if (msgResponse == -1) {
            printf("Error during weating response from ACCEPT\n");
            return -1;
        }

        if (msgResponse == 0) {
            waitResponse = 0;
        }
    }
    
    if (response.msg.data.action == PA_N) {
        return 1;
    }
    
    if (trade() == -1) {
        printf("Error during trade\n");
        return -1;
    }

    return 0;
}

int trade() {

    if (haveIGoodsToSell() == 0) {
        
        if (sellGoods() == -1) {
            printf("Error during selling goods\n");
            return -1;
        }
    }

    if (haveIGoodsToBuy() == 0) {
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

    if (sendMessage(currentMsgQueueId, boat.id, PA_EOT, 0, 0) == -1) {
        printf("Failed to send EOT comunication\n");
        return -1;
    }

    currentMsgQueueId = -1;

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

    /* Send request to sell */
    if (sendMessage(currentMsgQueueId, boat.id, PA_SE_GOOD, 0, 0) == -1) {
        printf("Failed to send PA_SE_GOOD comunication\n");
        return -1;
    }

    /* Wait response */
    int waitResponse = 1;
    PortMessage response;
    while (waitResponse == 1) {
        
        int msgResponse = reciveMessageById(currentMsgQueueId, boat.id, &response);
        if (msgResponse == -1) {
            printf("Error during weating response from PA_SE_GOOD\n");
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
    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", response.msg.data.semaphoreKey) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    sem_t *semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Boat failed to found semaphore with key %s\n", semaphoreKey);
        return -1;
    }

    /* Get shared memory of the port */
    Goods* arr = (Goods*) shmat(response.msg.data.sharedMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        printf("Error opening goods shared memory\n");
        return -1;
    }

    /* Sell all available goods */
    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (goodHold[i].loadInTon > 0 && goodHold[i].state != Expired_In_The_Boat) {
            
            if (arr[i].loadInTon == 0) {
                continue;
            }
            
            sem_wait(semaphore);

            int exchange = 0;

            /* If x >= 0 OK, x < 0 not enought good to sell */
            if (arr[i].loadInTon - goodHold[i].loadInTon >= 0) {
                exchange = goodHold[i].loadInTon;
                arr[i].loadInTon -= goodHold[i].loadInTon;
                goodHold[i].loadInTon = 0;
            } else {
                exchange = goodHold[i].loadInTon - arr[i].loadInTon;
                arr[i].loadInTon = 0;
                goodHold[i].loadInTon -= exchange;
            }

            sem_post(semaphore);

            if(waitExchange(exchange / configArr[SO_LOADSPEED]) == -1) {
                printf("Error in start nanosleep\n");
                return -1;
            }
        }    
    }

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the good semaphore\n");
        return -1;
    }

    return 0;
}

int buyGoods() {

    /* Send request to buy */
    if (sendMessage(currentMsgQueueId, boat.id, PA_RQ_GOOD, 0, 0) == -1) {
        printf("Failed to send PA_RQ_GOOD comunication\n");
        return -1;
    }

    /* Wait response */
    int waitResponse = 1;
    PortMessage response;
    while (waitResponse == 1) {
        
        int msgResponse = reciveMessageById(currentMsgQueueId, boat.id, &response);
        if (msgResponse == -1) {
            printf("Error during weating response from PA_RQ_GOOD\n");
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
    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", response.msg.data.semaphoreKey) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    sem_t *semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Boat failed to found semaphore with key %s\n", semaphoreKey);
        return -1;
    }

    /* Get shared memory of the port */
    Goods* arr = (Goods*) shmat(response.msg.data.sharedMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        printf("Error opening goods shared memory\n");
        return -1;
    }

    /* Buy some available goods */
    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if (arr[i].loadInTon > 0 && arr[i].state != Expired_In_The_Port) {
            
            sem_wait(semaphore);

            int availableSpace = getSpaceAvailableInTheHold();
            int exchange = 0;

            /* If x >= 0 OK, x < 0 not enought good to buy */
            if (arr[i].loadInTon - availableSpace >= 0) {
                exchange = availableSpace;
                arr[i].loadInTon -= availableSpace;
                goodHold[i].loadInTon += availableSpace;
            } else {
                exchange = availableSpace - arr[i].loadInTon;
                arr[i].loadInTon = 0;
                goodHold[i].loadInTon += exchange;
            }

            sem_post(semaphore);

            if(waitExchange(exchange / configArr[SO_LOADSPEED]) == -1) {
                printf("Error in start nanosleep\n");
                return -1;
            }
            
            if (getSpaceAvailableInTheHold() == 0) {
                break;
            }
        }    
    }

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the good semaphore\n");
        return -1;
    }
    
    return 0;
}

int waitExchange(int timeToSleep) {

    struct timespec ts1, ts2;
    ts1.tv_sec = 0;
    ts1.tv_nsec = (long) timeToSleep;

    if (nanosleep(&ts1 , &ts2) < 0) {
        printf("Nano sleep system call failed \n");
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

    printf("Recived boat end\n"); 

    return 0;
}