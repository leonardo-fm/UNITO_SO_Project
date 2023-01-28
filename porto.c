#define _GNU_SOURCES

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  
#include <sys/msg.h>   

#include <semaphore.h>
#include <fcntl.h>      

#include <signal.h>

#include "lib/config.h"
#include "lib/utilities.h"
#include "lib/msgPortProtocol.h"

#include "porto.h"

int handlePA_ACCEPT(PortMessage recivedMsg);
int handlePA_SE_GOOD(PortMessage recivedMsg);
int handlePA_RQ_GOOD(PortMessage recivedMsg);

int NUM_OF_SETTINGS = 13;
int* configArr;
 
int goodStockShareMemoryId;
int goodRequestShareMemoryId;

Port port;
Goods** goodExchange;

int startSimulation = 0;
int simulationRunning = 1;

void handle_port_start() {

    startSimulation = 1;
}

void handle_port_newDay() {

    newDay();
}

void handle_port_stopSimulation() {

    simulationRunning = 0;
}


int main(int argx, char* argv[]) {

    (void) argx;
    initializeEnvironment();
    initializeSingalsHandlers();

    if (initializeConfig(argv[0]) == -1) {
        printf("Initialization of port config failed\n");
        exit(1);
    }

    if (initializePort(argv[1], argv[2], argv[3]) == -1) {
        printf("Initialization of port %s failed\n", argv[1]);
        exit(2);
    }

    if (work() == -1) {
        printf("Error during port %d work\n", port.id);
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

    struct sigaction sa1;
    sa1.sa_flags = SA_RESTART;
    sa1.sa_handler = &handle_port_start;
    sigaction(SIGUSR1, &sa1, NULL);

    struct sigaction sa2;
    sa2.sa_flags = SA_RESTART;
    sa2.sa_handler = &handle_port_newDay;
    sigaction(SIGUSR2, &sa2, NULL);

    struct sigaction sa3;
    sa3.sa_flags = SA_RESTART;
    sa3.sa_handler = &handle_port_stopSimulation;
    sigaction(SIGTERM, &sa3, NULL);

    return 0;
}

/* Recover the array of the configurations values */
int initializeConfig(char* configShareMemoryIdString) {

    char* p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        return -1;
    }

    return 0;
}

int initializePort(char* portIdString, char* portShareMemoryIdS, char* goodShareMemoryIdS) {

    if (initializePortStruct(portIdString, portShareMemoryIdS) == -1) {
        printf("Error occurred during init of port struct\n");
        return -1;
    }

    if (initializeExchangeGoods() == -1) {
        printf("Error occurred during init of goods exchange\n");
        return -1;
    }

    if (initializePortGoods(goodShareMemoryIdS) == -1) {
        printf("Error occurred during init of goods\n");
        return -1;
    }

    return 0;
}

int initializePortStruct(char* portIdString, char* portShareMemoryIdS) {
        
    char* p;
    int portId = strtol(portIdString, &p, 10);

    /* Generate a message queue to comunicate with the boats */
    char queueKey[12];
    if (sprintf(queueKey, "%d", portId) == -1) {
        printf("Error during conversion of the port id to a string\n");
        return -1;
    }

    int portMsgId = msgget(ftok(queueKey, 'X'), IPC_CREAT | 0600);
    if (portMsgId == -1) {
        return -1;
    }

    port.id = portId;
    port.msgQueuId = portMsgId;
    if (port.id < 4) {
        port.position = getCornerCoordinates(configArr[SO_LATO], configArr[SO_LATO], port.id);
    } else {
        port.position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    }
    port.quays = getRandomValue(4, configArr[SO_BANCHINE]);
    port.availableQuays = port.quays;

    int shareMemoryId = strtol(portShareMemoryIdS, &p, 10);
    Coordinates* arr = (Coordinates*) shmat(shareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        return -1;
    }

    arr[port.id] = port.position;

    return 0;
}

int initializeExchangeGoods() {

    /* Generate shared memory for good stock */
    goodStockShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodStockShareMemoryId == -1) {
        printf("Error during creation of shared memory for goods stock\n");
        return -1;
    }
    Goods* arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        printf("Error during good memory allocation\n"); /*TODO ERRORE SBAGLIATO*/
        return -1;
    }
    if (generateSemaphore(goodStockShareMemoryId) == -1) {
        printf("Error during creation of semaphore for goods stock\n");
        return -1;
    }

    /* Generate shared memory for good request */
    goodRequestShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodStockShareMemoryId == -1) {
        printf("Error during creation of shared memory for goods request\n");
        return -1;
    }
    Goods* arrReques = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrReques == (void*) -1) {
        printf("Error during good memory allocation\n"); /*TODO ERRORE SBAGLIATO*/
        return -1;
    }
    if (generateSemaphore(goodRequestShareMemoryId) == -1) {
        printf("Error during creation of semaphore for goods request\n");
        return -1;
    }

    int maxRequest = (configArr[SO_FILL] / 2) / configArr[SO_MERCI] / configArr[SO_PORTI];
    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        Goods goodInStock;
        goodInStock.id = i;
        goodInStock.loadInTon = 0;
        goodInStock.state = In_The_Port;

        arrStock[i] = goodInStock;


        Goods goodRequested;
        goodRequested.id = i;
        goodRequested.loadInTon = maxRequest;
        goodRequested.state = In_The_Port;

        arrReques[i] = goodRequested;
    }

    return 0;
}

int initializePortGoods(char* goodShareMemoryIdS) {

    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", getppid()) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    sem_t *semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("port failed to found semaphore with key %s\n", semaphoreKey);
        return -1;
    }

    /* Generate an array of random number with no repetitions */
    int numOfGoods = configArr[SO_MERCI] / 2;
    int goodsToTake[numOfGoods];
    int i, j = 0;
    for (i = 0; i < numOfGoods; i++) {
        int flag = 1;
        int randomValue;
        while (flag == 1) {
            flag = 0;
            randomValue = getRandomValue(0, (configArr[SO_MERCI] - 1));
            for (j = 0; j < numOfGoods; j++) {
                if (goodsToTake[j] == randomValue) {
                    flag = 1;
                    break;
                }
            }
        }

        goodsToTake[i] = randomValue;
    }

    char* p;
    int shareMemoryId = strtol(goodShareMemoryIdS, &p, 10);
    Goods* arr = (Goods*) shmat(shareMemoryId, NULL, 0);
    if (arr == (void*) -1) {
        printf("Error opening goods shared memory\n");
        return -1;
    }

    int maxTake = (configArr[SO_FILL] / 2) / ((configArr[SO_MERCI] / 2) - 1) / configArr[SO_PORTI];

    sem_wait(semaphore);

    Goods* arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        printf("Error while opening stock good\n");
        return -1;
    }

    Goods* arrReques = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrReques == (void*) -1) {
        printf("Error while opening request good\n");
        return -1;
    }

    for (i = 0; i < numOfGoods; i++) {
        int currentGood = goodsToTake[i];

        /* Set life span */
        arrStock[currentGood].remaningDays = arr[currentGood].remaningDays;
        arrReques[currentGood].remaningDays = arr[currentGood].remaningDays;

        if (arr[currentGood].loadInTon == 0) {
            continue;
        }

        if (arr[currentGood].loadInTon < maxTake) {
            arrStock[currentGood].loadInTon += arr[currentGood].loadInTon;
            arr[currentGood].loadInTon = 0;
        } else {
            arr[currentGood].loadInTon -= maxTake;
            arrStock[currentGood].loadInTon += maxTake;
        }

        /* Set to 0 the request for the current good in the port */
        arrReques[currentGood].loadInTon = 0;
    }

    sem_post(semaphore);

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the good semaphore\n");
        return -1;
    }

    return 0;
}

int work() {

    /* wait for simulation to start */
    while (startSimulation == 0) { };

    while (simulationRunning == 1)
    {
        PortMessage recivedMsg;
        int msgStatus = reciveMessage(port.msgQueuId, PORT_RECEIVER, &recivedMsg);

        if (msgStatus == -1) {
            printf("Error during reciving message from boat\n");
            return -1;
        }

        if (msgStatus == 0) {
            
            /* New message */
            switch (recivedMsg.msg.data.action)
            {
                case PA_ACCEPT:
                    if (handlePA_ACCEPT(recivedMsg) == -1) {
                        printf("Error during ACCEPT handling\n");
                        return -1;
                    };
                    break;
                case PA_RQ_GOOD:
                    if (handlePA_RQ_GOOD(recivedMsg) == -1) {
                        printf("Error during RQ_GOOD handling\n");
                        return -1;
                    };
                    break;
                case PA_SE_GOOD:
                    if (handlePA_SE_GOOD(recivedMsg) == -1) {
                        printf("Error during SE_GOOD handling\n");
                        return -1;
                    };
                    break;
                case PA_EOT:
                    if (handlePA_EOT() == -1) {
                        printf("Error during EOT handling\n");
                        return -1;
                    };
                    break;
                default:
                    break;
            }
        } 
    }

    return 0;
}

int newDay() {

    Goods* arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        printf("Error while opening stock good\n");
        return -1;
    }

    int i = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(arrStock[i].remaningDays > 0) {
            arrStock[i].remaningDays--;
            if (arrStock[i].remaningDays == 0) {
                arrStock[i].state = Expired_In_The_Port;
            }
        }
    }

    /* TODO Dealy dump */

    return 0;
}

int handlePA_ACCEPT(PortMessage recivedMsg) {
    printf("Handle ACCEPT request\n");
    if (port.availableQuays > 0) {
        if (sendMessage(port.msgQueuId, recivedMsg.msg.data.id, BOAT_RECEIVER, PA_Y, -1, -1) == -1) {
            printf("Error during send ACCEPT\n");
            return -1;
        }
        port.availableQuays--;
    } else {
        if (sendMessage(port.msgQueuId, recivedMsg.msg.data.id, BOAT_RECEIVER, PA_N, -1, -1) == -1) {
            printf("Error during send ACCEPT\n");
            return -1;
        }
    }

    return 0;
}

int handlePA_SE_GOOD(PortMessage recivedMsg) {
    /* The boat want to sell some goods */
    if (sendMessage(port.msgQueuId, recivedMsg.msg.data.id, BOAT_RECEIVER, PA_Y, 
        goodRequestShareMemoryId, goodRequestShareMemoryId) == -1) {
            printf("Error during send SE_GOOD\n");
            return -1;
        }
    
    return 0;
}

int handlePA_RQ_GOOD(PortMessage recivedMsg) {
    /* The boat want to buy some goods */
    if (sendMessage(port.msgQueuId, recivedMsg.msg.data.id, BOAT_RECEIVER, PA_Y, 
        goodStockShareMemoryId, goodStockShareMemoryId) == -1) {
            printf("Error during send RQ_GOOD\n");
            return -1;
        }

    return 0;
}

int handlePA_EOT() {
    port.availableQuays++;

    return 0;
}

int generateShareMemory(int sizeOfSegment) {
    int shareMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (shareMemoryId == -1) {
        printf("Error during creation of the shared memory\n");
        return -1;
    }

    return shareMemoryId;
}

/* Generate a semaphore */
/* https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes */
int generateSemaphore(int semKey) {

    char semaphoreKey[12];
    if (sprintf(semaphoreKey, "%d", semKey) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    sem_t* semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Error on opening the semaphore\n");
        return -1;
    }

    if (sem_close(semaphore) < 0) {
        printf("Error on closing the semaphore\n");
        return -1;
    }

    return 0;
}

int cleanup() {

    printf("Recived port end\n"); 
    
    if (msgctl(port.msgQueuId, IPC_RMID, NULL) == -1) {
        printf("The queue failed to be closed\n");
        return -1;
    }

    if (shmctl(goodStockShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }

    if (shmctl(goodRequestShareMemoryId, IPC_RMID, NULL) == -1) {
        printf("The shared memory failed to be closed\n");
        return -1;
    }

    return 0;
}