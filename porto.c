#define _GNU_SOURCES

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  
#include <sys/msg.h>   
#include <errno.h>

#include <semaphore.h>
#include <fcntl.h>      

#include <signal.h>

#include "lib/config.h"
#include "lib/utilities.h"
#include "lib/msgPortProtocol.h"

#include "porto.h"

int NUM_OF_SETTINGS = 13;
int *configArr;
 
int goodStockShareMemoryId;
int goodRequestShareMemoryId;

Port port;
Goods **goodExchange;

int simulationRunning = 1;

void handle_port_simulation_signals(int signal) {

    switch (signal)
    {
        /* Start of the simulation */
        case SIGUSR1:
            break;
            
        /* New day of simulation */
        case SIGUSR2:
            newDay();
            break;

        /* End of the simulation */
        case SIGSYS:
            simulationRunning = 0;
            stopWaitingQueues = 1;
            break;
        default:
            printf("Intercept a unhandled signal: %d\n", signal);
            break;
    }
}

void handle_port_stopProcess() {

    cleanup();
    exit(0);
}

int main(int argx, char *argv[]) {

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

    struct sigaction signalAction;

    setpgid(getpid(), getppid());

    signal(SIGUSR1, handle_port_simulation_signals);

    /* Use different mwthod because i need to use the handler multiple times */
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_port_simulation_signals;
    sigaction(SIGUSR2, &signalAction, NULL);

    signal(SIGSYS, handle_port_simulation_signals);
    signal(SIGINT, handle_port_stopProcess);

    return 0;
}

/* Recover the array of the configurations values */
int initializeConfig(char *configShareMemoryIdString) {

    char *p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        return -1;
    }

    return 0;
}

int initializePort(char *portIdString, char *portShareMemoryIdS, char *goodShareMemoryIdS) {

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

int initializePortStruct(char *portIdString, char *portShareMemoryIdS) {
        
    char *p;
    char queueKey[12];
    int portId, portMsgId, shareMemoryId;
    Port *arrPort;

    portId = strtol(portIdString, &p, 10);
    
    /* Generate a message queue to comunicate with the boats */
    if (sprintf(queueKey, "%d", portId) == -1) {
        printf("Error during conversion of the port id to a string\n");
        return -1;
    }

    portMsgId = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
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

    shareMemoryId = strtol(portShareMemoryIdS, &p, 10);
    arrPort = (Port*) shmat(shareMemoryId, NULL, 0);
    if (arrPort == (void*) -1) {
        return -1;
    }

    arrPort[port.id] = port;

    if (shmdt(arrPort) == -1) {
        printf("The arr init port detach failed\n");
        return -1;
    }

    return 0;
}

int initializeExchangeGoods() {

    Goods *arrStock, *arrReques;
    int maxRequest, i;

    /* Generate shared memory for good stock */
    goodStockShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodStockShareMemoryId == -1) {
        printf("Error during creation of shared memory for goods stock\n");
        return -1;
    }
    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        printf("Error during semaphore creation\n");
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
    arrReques = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrReques == (void*) -1) {
        printf("Error during semaphore creation\n");
        return -1;
    }
    if (generateSemaphore(goodRequestShareMemoryId) == -1) {
        printf("Error during creation of semaphore for goods request\n");
        return -1;
    }

    maxRequest = (configArr[SO_FILL] / 2) / configArr[SO_MERCI] / configArr[SO_PORTI];
    for (i = 0; i < configArr[SO_MERCI]; i++) {

        Goods goodInStock, goodRequested;

        goodInStock.id = i;
        goodInStock.loadInTon = 0;
        goodInStock.state = In_The_Port;

        arrStock[i] = goodInStock;

        goodRequested.id = i;
        goodRequested.loadInTon = maxRequest;
        goodRequested.state = In_The_Port;

        arrReques[i] = goodRequested;
    }

    if (shmdt(arrStock) == -1) {
        printf("The arr stock detach failed\n");
        return -1;
    }

    if (shmdt(arrReques) == -1) {
        printf("The arr request detach failed\n");
        return -1;
    }

    return 0;
}

int initializePortGoods(char *goodShareMemoryIdS) {

    char semaphoreKey[12];
    sem_t *semaphore;

    int i, j, shareMemoryId, maxTake;
    int *goodsToTake;

    char *p;
    Goods *arrGood, *arrStock, *arrReques; 
    
    if (sprintf(semaphoreKey, "%d", getppid()) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("port failed to found semaphore with key %s\n", semaphoreKey);
        return -1;
    }

    /* Generate an array of random number with no repetitions */
    goodsToTake = (int*) malloc(sizeof(int) * configArr[SO_MERCI]);

    for (i = 0; i < configArr[SO_MERCI] - 1; i++) {

        int flag, randomValue;
        
        flag = 1;
        while (flag == 1) {
            flag = 0;
            randomValue = getRandomValue(0, (configArr[SO_MERCI] - 1));
            for (j = 0; j < configArr[SO_MERCI]; j++) {
                if (goodsToTake[j] == randomValue) {
                    flag = 1;
                    break;
                }
            }
        }

        goodsToTake[i] = randomValue;
    }

    shareMemoryId = strtol(goodShareMemoryIdS, &p, 10);
    arrGood = (Goods*) shmat(shareMemoryId, NULL, 0);
    if (arrGood == (void*) -1) {
        printf("Error opening goods shared memory\n");
        return -1;
    }

    maxTake = (configArr[SO_FILL] / 2) / ((configArr[SO_MERCI] / 2) - 1) / configArr[SO_PORTI];

    sem_wait(semaphore);

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        printf("Error while opening stock good\n");
        return -1;
    }

    arrReques = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrReques == (void*) -1) {
        printf("Error while opening request good\n");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        int currentGood = goodsToTake[i];

        /* Set life span */
        arrStock[currentGood].remaningDays = arrGood[currentGood].remaningDays;
        arrReques[currentGood].remaningDays = arrGood[currentGood].remaningDays;

        if (arrGood[currentGood].loadInTon == 0) {
            continue;
        }

        if (arrGood[currentGood].loadInTon < maxTake) {
            arrStock[currentGood].loadInTon += arrGood[currentGood].loadInTon;
            arrGood[currentGood].loadInTon = 0;
        } else {
            arrGood[currentGood].loadInTon -= maxTake;
            arrStock[currentGood].loadInTon += maxTake;
        }

        /* Set to 0 the request for the current good in the port */
        arrReques[currentGood].loadInTon = 0;
    }

    free(goodsToTake);

    sem_post(semaphore);

    if (sem_close(semaphore) < 0) {
        printf("Error unable to close the good semaphore\n");
        return -1;
    }

    if (shmdt(arrGood) == -1) {
        printf("The arr goods detach failed\n");
        return -1;
    }

    if (shmdt(arrStock) == -1) {
        printf("The arr stock detach failed\n");
        return -1;
    }

    if (shmdt(arrReques) == -1) {
        printf("The arr request detach failed\n");
        return -1;
    }

    return 0;
}

int work() {

    int maxQauys, i, j;
    int *queues[2];
    
    /* wait for simulation to start */
    if (waitForStart() != 0) {
        printf("Error while waiting for start\n");
        return -1;
    }

    /* queue[0][n] read queue | queue[1][n] write queue */
    maxQauys = port.availableQuays;
    queues[0] = (int*) malloc(sizeof(int) * maxQauys);
    queues[1] = (int*) malloc(sizeof(int) * maxQauys);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < maxQauys; j++) {
            queues[i][j] = -1;
        }
    }

    /* Remains alive until all the quays are empty to avoid boats closing errors */
    while (simulationRunning == 1 || (port.quays - port.availableQuays) > 0)
    {
        if (simulationRunning == 1) {
            PortMessage setupMsg;
            int flag = port.availableQuays == port.quays ? 0 : IPC_NOWAIT;
            int setupMsgStatus = receiveMessage(port.msgQueuId, &setupMsg, flag, 1);

            if (setupMsgStatus == -1) {
                printf("Error during reciving message from boat\n");
                return -1;
            }
            if (setupMsgStatus == 0 && setupMsg.msg.data.action == PA_SETUP) {

                for (j = 0; j < maxQauys; j++) { 
                    if (queues[0][j] == -1) {
                        queues[0][j] = setupMsg.msg.data.data1;
                        queues[1][j] = setupMsg.msg.data.data2;
                        break;
                    }
                }
            }
        }

        for (j = 0; j < maxQauys; j++) {

            int readingMsgQueue, writingMsgQueue, msgStatus;
            PortMessage receivedMsg;

            if (queues[0][j] == -1) {
                continue;
            }

            readingMsgQueue = queues[0][j];
            writingMsgQueue = queues[1][j];

            msgStatus = receiveMessage(readingMsgQueue, &receivedMsg, 0, 0);

            if (msgStatus == -1) {
                printf("Error during reciving message (r: %d, w: %d) from boat\n", readingMsgQueue, writingMsgQueue);
                return -1;
            }

            if (msgStatus == 0) {
                
                /* New message */
                int acceptResponse;
                switch (receivedMsg.msg.data.action)
                {
                    case PA_ACCEPT:
                        acceptResponse = handlePA_ACCEPT(writingMsgQueue);
                        if (acceptResponse == -1) {
                            printf("Error during ACCEPT handling\n");
                            return -1;
                        };
                        if (acceptResponse == 1) {
                            queues[0][j] = -1;
                            queues[1][j] = -1;
                        }
                        break;
                    case PA_SE_GOOD:
                        if (handlePA_SE_GOOD(writingMsgQueue) == -1) {
                            printf("Error during SE_GOOD handling\n");
                            return -1;
                        };
                        break;
                    case PA_RQ_GOOD:
                        if (handlePA_RQ_GOOD(writingMsgQueue) == -1) {
                            printf("Error during RQ_GOOD handling\n");
                            return -1;
                        };
                        break;
                    case PA_EOT:
                        if (handlePA_EOT(writingMsgQueue) == -1) {
                            printf("Error during EOT handling\n");
                            return -1;
                        };
                        queues[0][j] = -1;
                        queues[1][j] = -1;
                        break;
                    default:
                        break;
                }
            } 
        }
    }

    if (freePendingMsgs() == -1) {
        printf("Error while freeing pending messages\n");
        return -1;
    }

    free(queues[0]);
    free(queues[1]);

    return 0;
}

int waitForStart() {

    int sig, waitRes;
    sigset_t sigset;

    sigaddset(&sigset, SIGUSR1);
    waitRes = sigwait(&sigset, &sig);

    return waitRes;
}

int freePendingMsgs() {

    /* Check for pending requests */
    struct msqid_ds msgInfo;
    int msgInfoResponse, pendingMsg, i;
    
    msgInfoResponse = msgctl(port.msgQueuId, IPC_STAT, &msgInfo);
    if (msgInfoResponse == -1) {
        printf("Error while retrieving info about message queue\n");
        return -1;
    }

    pendingMsg = (int) msgInfo.msg_qnum;  
    for (i = 0; i < pendingMsg; i++) {
        PortMessage pendingMsg;
        int pendingMsgStatus = receiveMessage(port.msgQueuId, &pendingMsg, 0, 0);

        if (pendingMsgStatus == -1) {
            printf("Error during reciving message from boat on pending messages\n");
            return -1;
        }
        
        if (sendMessage(pendingMsg.msg.data.data2, PA_N, -1, -1) == -1) {
            printf("Error during send NO\n");
            return -1;
        }
    }

    return 0;
}

int newDay() {
    
    int i;
    Goods *arrStock;

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        printf("Error while opening stock good\n");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(arrStock[i].remaningDays > 0) {
            arrStock[i].remaningDays--;
            if (arrStock[i].remaningDays == 0) {
                arrStock[i].state = Expired_In_The_Port;
            }
        }
    }

    if (shmdt(arrStock) == -1) {
        printf("The arr stock detach failed\n");
        return -1;
    }

    /* TODO Dealy dump */

    return 0;
}

/* If the port accept the boat request return 0, if not return 1, for errors return -1 */
int handlePA_ACCEPT(int queueId) {

    if (port.availableQuays > 0 && simulationRunning == 1) {
        if (sendMessage(queueId, PA_Y, -1, -1) == -1) {
            printf("Error during send ACCEPT\n");
            return -1;
        }
        port.availableQuays--;
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            printf("Error during send ACCEPT\n");
            return -1;
        }

        return 1;
    }

    return 0;
}

int handlePA_SE_GOOD(int queueId) {

    if (simulationRunning == 1) {
        /* The boat want to sell some goods */
        if (sendMessage(queueId, PA_Y, goodRequestShareMemoryId, goodRequestShareMemoryId) == -1) {
                printf("Error during send SE_GOOD errno: %d\n", errno);
                return -1;
        }
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            printf("Error during send SE NO\n");
            return -1;
        }
    }
    
    return 0;
}

int handlePA_RQ_GOOD(int queueId) {

    if (simulationRunning == 1) {
        /* The boat want to buy some goods */
        if (sendMessage(queueId, PA_Y, goodStockShareMemoryId, goodStockShareMemoryId) == -1) {
                printf("Error during send RQ_GOOD errno: %d\n", errno);
                return -1;
        }
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            printf("Error during send RQ NO\n");
            return -1;
        }
    }

    return 0;
}

int handlePA_EOT(int writeQueueId) {

    /* The port acknowledge to end the transmission */
    if (sendMessage(writeQueueId, PA_EOT, 0, 0) == -1) {
            printf("Error during send EOT errno: %d\n", errno);
            return -1;
    }

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
    sem_t *semaphore;

    if (sprintf(semaphoreKey, "%d", semKey) == -1) {
        printf("Error during conversion of the pid for semaphore to a string\n");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        printf("Error on opening the semaphore\n");
        return -1;
    }

    return 0;
}

int cleanup() {

    if (shmdt(configArr) == -1) {
        printf("The conf arr detach failed\n");
        return -1;
    }

    if (msgctl(port.msgQueuId, IPC_RMID, NULL) == -1) {
        printf("PORT The queue failed to be closed\n");
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