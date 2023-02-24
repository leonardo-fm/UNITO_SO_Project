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

#include "lib/porto.h"

int *configArr;

int goodAnalyzerSharedMemoryId; 
int portAnalyzerSharedMemoryId; 
 
int goodStockShareMemoryId;
int goodRequestShareMemoryId;

int endGoodSharedMemoryId; 

Port port;
Goods **goodExchange;

int simulationRunning = 1;

void handle_port_simulation_signals(int signal) {

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
            stopWaitingQueues = 1;
            break;
        default:
            handleError("Intercept a unhandled signal");
            break;
    }
}

void handle_port_stopProcess() {

    cleanup();
    exit(0);
}

/* argv[0]=id | argv[1]=configsh | argv[2]=portsh | argv[3]=goodsh | 
    argv[4]=ganalizersh | argv[5]=pc analyzersh | argv[6]=akish | argv[7]=endanalyzershm */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeEnvironment();
    initializeSingalsHandlers();

    if (initializeConfig(argv[1], argv[4], argv[5]) == -1) {
        handleError("Initialization of port config failed");
        safeExit(1);
    }

    if (initializePort(argv[0], argv[2], argv[3], argv[6], argv[7]) == -1) {
        handleError("Initialization of port failed");
        safeExit(2);
    }

    if (work() == -1) {
        handleError("Error during port work");
        safeExit(3);
    }

    if (cleanup() == -1) {
        handleError("Port cleanup failed");
        safeExit(4);
    }

    return 0;
}

int initializeSingalsHandlers() {

    struct sigaction signalAction;

    setpgid(getpid(), getppid());

    signal(SIGUSR1, handle_port_simulation_signals);

    /* Use different method because i need to use the handler multiple times */
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_port_simulation_signals;
    sigaction(SIGUSR2, &signalAction, NULL);

    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_port_simulation_signals;
    sigaction(SIGCONT, &signalAction, NULL);

    signal(SIGSYS, handle_port_simulation_signals);
    signal(SIGINT, handle_port_stopProcess);

    return 0;
}

/* Recover the array of the configurations values */
int initializeConfig(char *configShareMemoryIdString, char *goodAnalyzerShareMemoryIdString, char *portAnalyzerShareMemoryIdString) {

    char *p;
    int configShareMemoryId = strtol(configShareMemoryIdString, &p, 10);
    
    goodAnalyzerSharedMemoryId = strtol(goodAnalyzerShareMemoryIdString, &p, 10);
    portAnalyzerSharedMemoryId = strtol(portAnalyzerShareMemoryIdString, &p, 10);
    
    configArr = (int*) shmat(configShareMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    return 0;
}

int initializePort(char *portIdString, char *portShareMemoryIdS, char *goodShareMemoryIdS, 
    char *acknowledgeInitShareMemoryIdS, char *endGoodShareMemoryIdS) {
    
    int acknowledgeInitShareMemoryId;
    char *p;
    int *acknowledgeArr;

    if (initializePortStruct(portIdString, portShareMemoryIdS) == -1) {
        handleError("Error occurred during init of port struct");
        return -1;
    }

    if (initializeExchangeGoods() == -1) {
        handleError("Error occurred during init of goods exchange");
        return -1;
    }

    if (initializePortGoods(goodShareMemoryIdS, endGoodShareMemoryIdS) == -1) {
        handleError("Error occurred during init of goods");
        return -1;
    }

    acknowledgeInitShareMemoryId = strtol(acknowledgeInitShareMemoryIdS, &p, 10);
    acknowledgeArr = (int*) shmat(acknowledgeInitShareMemoryId, NULL, 0);
    if (acknowledgeArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    acknowledgeArr[port.id] = 1;

    if (shmdt(acknowledgeArr) == -1) {
        handleErrno("shmdt()");
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
        handleError("Error during conversion of the port id to a string");
        return -1;
    }

    portMsgId = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (portMsgId == -1) {
        handleErrno("msgget()");
        return -1;
    }

    port.id = portId;
    port.msgQueuId = portMsgId;
    if (port.id < 4) {
        port.position = getCornerCoordinates(configArr[SO_LATO], configArr[SO_LATO], port.id);
    } else {
        port.position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    }
    port.quays = getRandomValue(1, configArr[SO_BANCHINE]);
    port.availableQuays = port.quays;

    shareMemoryId = strtol(portShareMemoryIdS, &p, 10);
    arrPort = (Port*) shmat(shareMemoryId, NULL, 0);
    if (arrPort == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrPort[port.id] = port;

    if (shmdt(arrPort) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int initializeExchangeGoods() {

    Goods *arrStock, *arrRequest;
    int maxRequest, i;

    /* Generate shared memory for good stock */
    goodStockShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodStockShareMemoryId == -1) {
        handleError("Error during creation of shared memory for goods stock");
        return -1;
    }

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    if (generateSemaphore(goodStockShareMemoryId) == -1) {
        handleError("Error during creation of semaphore for goods stock");
        return -1;
    }

    /* Generate shared memory for good request */
    goodRequestShareMemoryId = generateShareMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodRequestShareMemoryId == -1) {
        handleError("Error during creation of shared memory for goods request");
        return -1;
    }

    arrRequest = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrRequest == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }
    
    if (generateSemaphore(goodRequestShareMemoryId) == -1) {
        handleError("Error during creation of semaphore for goods request");
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

        arrRequest[i] = goodRequested;
    }

    if (shmdt(arrStock) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrRequest) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int initializePortGoods(char *goodShareMemoryIdS, char *endGoodShareMemoryIdS) {

    int i, goodShareMemoryId;

    char *p;
    Goods *arrMasterGood, *arrStock, *arrRequest; 
    goodEndDump *arrEndGoodDump;
    sem_t *endGoodSemaphore;
    char semaphoreKey[12];

    goodShareMemoryId = strtol(goodShareMemoryIdS, &p, 10);
    arrMasterGood = (Goods*) shmat(goodShareMemoryId, NULL, 0);
    if (arrMasterGood == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrRequest = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrRequest == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    endGoodSharedMemoryId = strtol(endGoodShareMemoryIdS, &p, 10);
    arrEndGoodDump = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (arrEndGoodDump == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    if (sprintf(semaphoreKey, "%d", endGoodSharedMemoryId) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }

    endGoodSemaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (endGoodSemaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        
        int stockOrRequest = getRandomValue(0, 1);


        if (stockOrRequest == 0) {
            /* Fill stock */
            arrStock[i].remaningDays = arrMasterGood[i].remaningDays;
            arrStock[i].loadInTon = arrMasterGood[i].loadInTon;
            arrRequest[i].loadInTon = 0;
            arrStock[i].state = In_The_Port;
            
            sem_wait(endGoodSemaphore);
            arrEndGoodDump[i].totalInitNumber += arrStock[i].loadInTon;
            sem_post(endGoodSemaphore);
        } else {
            /* Fill request */
            arrRequest[i].remaningDays = arrMasterGood[i].remaningDays;
            arrRequest[i].loadInTon = arrMasterGood[i].loadInTon;
            arrStock[i].loadInTon = 0;
            arrRequest[i].state = In_The_Port;
        }

        arrStock[i].dailyExchange = 0;
        arrRequest[i].dailyExchange = 0;
    }

    if (sem_close(endGoodSemaphore) < 0) {
        handleErrno("sem_close()");
        return -1;
    }

    if (shmdt(arrEndGoodDump) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrMasterGood) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrStock) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrRequest) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int work() {

    int maxQauys, i, j;
    int *queues[2];
    Goods *arrStock;
    goodEndDump *arrEndGoodDump;
    sem_t *endGoodSemaphore;
    char semaphoreKey[12];

    /* wait for simulation to start */
    if (waitForStart() != 0) {
        handleError("Error while waiting for start");
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
                handleError("Error during reciving message from boat");
                return -1;
            }

            if (setupMsgStatus == 0 && setupMsg.msg.data.action == PA_SETUP) {
                
                int portHasAccepted = 0;
                for (j = 0; j < maxQauys; j++) { 
                    if (queues[0][j] == -1) {
                        queues[0][j] = setupMsg.msg.data.data1;
                        queues[1][j] = setupMsg.msg.data.data2;
                        portHasAccepted = 1;
                        break;
                    }
                }

                if (portHasAccepted == 0) {
                    /* All the quays are full */
                    sendMessage(setupMsg.msg.data.data2, PA_N, -1, -1);
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
                handleError("Error during reciving message from boat");
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
                            handleError("Error during ACCEPT handling");
                            return -1;
                        };
                        if (acceptResponse == 1) {
                            queues[0][j] = -1;
                            queues[1][j] = -1;
                        }
                        break;
                    case PA_SE_GOOD:
                        if (handlePA_SE_GOOD(writingMsgQueue) == -1) {
                            handleError("Error during SE_GOOD handling");
                            return -1;
                        };
                        break;
                    case PA_SE_SUMMARY:
                        if (handlePA_SE_SUMMARY(receivedMsg.msg.data.data1, receivedMsg.msg.data.data2) == -1) {
                            handleError("Error during PA_SE_SUMMARY handling");
                            return -1;
                        };
                        break;
                    case PA_RQ_GOOD:
                        if (handlePA_RQ_GOOD(writingMsgQueue) == -1) {
                            handleError("Error during RQ_GOOD handling");
                            return -1;
                        };
                        break;
                    case PA_RQ_SUMMARY:
                        if (handlePA_RQ_SUMMARY(receivedMsg.msg.data.data1, receivedMsg.msg.data.data2) == -1) {
                            handleError("Error during PA_RQ_SUMMARY handling");
                            return -1;
                        };
                        break;
                    case PA_EOT:
                        if (handlePA_EOT(writingMsgQueue) == -1) {
                            handleError("Error during EOT handling");
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
        handleError("Error while freeing pending messages");
        return -1;
    }

    free(queues[0]);
    free(queues[1]);

    /* End good dump */
    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrEndGoodDump = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (arrEndGoodDump == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    if (sprintf(semaphoreKey, "%d", endGoodSharedMemoryId) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }

    endGoodSemaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (endGoodSemaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        sem_wait(endGoodSemaphore);
        arrEndGoodDump[i].inPort += arrStock[i].loadInTon;
        sem_post(endGoodSemaphore);
    }

    if (sem_close(endGoodSemaphore) < 0) {
        handleErrno("sem_close()");
        return -1;
    }

    if (shmdt(arrEndGoodDump) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrStock) == -1) {
        handleErrno("shmdt()");
        return -1;
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

int freePendingMsgs() {

    /* Check for pending requests */
    struct msqid_ds msgInfo;
    int msgInfoResponse, pendingMsg, i;
    
    msgInfoResponse = msgctl(port.msgQueuId, IPC_STAT, &msgInfo);
    if (msgInfoResponse == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    pendingMsg = (int) msgInfo.msg_qnum;  
    for (i = 0; i < pendingMsg; i++) {
        PortMessage pendingMsg;
        int pendingMsgStatus = receiveMessage(port.msgQueuId, &pendingMsg, 0, 0);

        if (pendingMsgStatus == -1) {
            handleError("Error during reciving message from boat on pending messages");
            return -1;
        }
        
        if (sendMessage(pendingMsg.msg.data.data2, PA_N, -1, -1) == -1) {
            handleError("Error during send NO");
            return -1;
        }
    }

    return 0;
}

int dumpData() {
    int i, totalGoodInStock, totalGoodRequested, goodReferenceId, totalDailyGoodsSold, totalDailyGoodsRecived;
    Goods *arrStock, *arrRequest;

    goodDailyDump *goodArr;
    goodDailyDump gdd;

    portDailyDump *portArr;
    portDailyDump pdd;

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrRequest = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrRequest == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    /* Init data */
    totalGoodInStock = 0;
    totalGoodRequested = 0;
    totalDailyGoodsSold = 0;
    totalDailyGoodsRecived = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(arrStock[i].state != Expired_In_The_Port) {
            totalGoodInStock += arrStock[i].loadInTon;
        }
        if(arrRequest[i].state != Expired_In_The_Port) {
            totalGoodRequested += arrRequest[i].loadInTon;
        }
        
        totalDailyGoodsSold += arrStock[i].dailyExchange;        
        totalDailyGoodsRecived += arrRequest[i].dailyExchange;
    }

    /* Send goods data */
    goodReferenceId = port.id * configArr[SO_MERCI];
    goodArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        
        gdd.goodId = i;
        if (arrStock[i].state == Expired_In_The_Port) {
            gdd.Good_Expired_In_The_Port = arrStock[i].loadInTon;
            
            arrStock[i].loadInTon = 0;
            arrStock[i].state = Undefined;
        } else {
            gdd.Good_Expired_In_The_Port = 0;
        }

        gdd.Good_In_The_Port = arrStock[i].loadInTon;
        gdd.Good_Delivered = arrRequest[i].dailyExchange;

        gdd.Good_Expired_In_The_Boat = 0;
        gdd.Good_In_The_Boat = 0;

        memcpy(&goodArr[goodReferenceId + i], &gdd, sizeof(goodDailyDump)); 
    }

    if (shmdt(goodArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    /* Send port data */
    portArr = (portDailyDump*) shmat(portAnalyzerSharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    pdd.id = port.id;
    pdd.totalGoodInStock = totalGoodInStock;
    pdd.totalGoodRequested = totalGoodRequested;
    pdd.totalGoodRecived = totalDailyGoodsRecived;
    pdd.totalGoodSold = totalDailyGoodsSold;
    pdd.totalQuays = port.quays;
    pdd.busyQuays = port.quays - port.availableQuays;

    portArr[port.id] = pdd;

    if (shmdt(portArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrStock) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrRequest) == -1) {
        handleErrno("shmdt()");
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
    Goods *arrStock, *arrRequest;
    char buffer[128];
    goodEndDump *arrEndGoodDump;
    sem_t *endGoodSemaphore;
    char semaphoreKey[12];

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrRequest = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrRequest == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrEndGoodDump = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (arrEndGoodDump == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    if (sprintf(semaphoreKey, "%d", endGoodSharedMemoryId) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }

    endGoodSemaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (endGoodSemaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    /* TODO nel mentre che avviene uno scambio con una nave, se la merce scade allora la nave la conta come da se e anche il porto */
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(arrStock[i].remaningDays > 0) {
            arrStock[i].remaningDays--;
            if (arrStock[i].remaningDays == 0) {
                arrStock[i].state = Expired_In_The_Port;

                sem_wait(endGoodSemaphore);
                arrEndGoodDump[i].expiredInPort += arrStock[i].loadInTon;
                sem_post(endGoodSemaphore);
            }
        }

        /* reset daily exchanges */
        arrStock[i].dailyExchange = 0;
        arrRequest[i].dailyExchange = 0;
    }

    if (sem_close(endGoodSemaphore) < 0) {
        handleErrno("sem_close()");
        return -1;
    }

    if (shmdt(arrEndGoodDump) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrStock) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrRequest) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "Port %d, free quays %d/%d", port.id, port.availableQuays, port.quays);
    debug(buffer);

    return 0;
}

/* If the port accept the boat request return 0, if not return 1, for errors return -1 */
int handlePA_ACCEPT(int queueId) {

    if (port.availableQuays > 0 && simulationRunning == 1) {
        if (sendMessage(queueId, PA_Y, -1, -1) == -1) {
            handleError("Error during send ACCEPT");
            return -1;
        }

        port.availableQuays--;

    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            handleError("Error during send ACCEPT");
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
            handleError("Error during send SE_GOOD");
            return -1;
        }
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            handleError("Error during send SE NO");
            return -1;
        }
    }
    
    return 0;
}

int handlePA_SE_SUMMARY(int goodId, int exchangeQuantity) {

    Goods *arrRequest;

    arrRequest = (Goods*) shmat(goodRequestShareMemoryId, NULL, 0);
    if (arrRequest == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrRequest[goodId].dailyExchange += exchangeQuantity;

    if (shmdt(arrRequest) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int handlePA_RQ_GOOD(int queueId) {

    if (simulationRunning == 1) {
        /* The boat want to buy some goods */
        if (sendMessage(queueId, PA_Y, goodStockShareMemoryId, goodStockShareMemoryId) == -1) {
            handleError("Error during send RQ_GOOD");
            return -1;
        }
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            handleError("Error during send RQ NO");
            return -1;
        }
    }

    return 0;
}

int handlePA_RQ_SUMMARY(int goodId, int exchangeQuantity) {

    Goods *arrStock;
    goodEndDump *arrEndGoodDump;
    sem_t *endGoodSemaphore;
    char semaphoreKey[12];

    arrStock = (Goods*) shmat(goodStockShareMemoryId, NULL, 0);
    if (arrStock == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    arrEndGoodDump = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (arrEndGoodDump == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    if (sprintf(semaphoreKey, "%d", endGoodSharedMemoryId) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }

    endGoodSemaphore = sem_open(semaphoreKey, O_EXCL, 0600, 1);
    if (endGoodSemaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    arrStock[goodId].dailyExchange += exchangeQuantity;
    
    sem_wait(endGoodSemaphore);
    arrEndGoodDump[goodId].exchanged += exchangeQuantity;
    sem_post(endGoodSemaphore);

    if (sem_close(endGoodSemaphore) < 0) {
        handleErrno("sem_close()");
        return -1;
    }

    if (shmdt(arrEndGoodDump) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (shmdt(arrStock) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    return 0;
}

int handlePA_EOT(int writeQueueId) {

    /* The port acknowledge to end the transmission */
    if (sendMessage(writeQueueId, PA_EOT, 0, 0) == -1) {
        handleError("Error during send EOT");
        return -1;
    }

    port.availableQuays++;

    return 0;
}

int generateShareMemory(int sizeOfSegment) {
    int shareMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (shareMemoryId == -1) {
        handleErrno("shmget()");
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
        handleError("Error during conversion of the pid for semaphore to a string");
        return -1;
    }   

    semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    return 0;
}

int cleanup() {

    if (shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (msgctl(port.msgQueuId, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    if (shmctl(goodStockShareMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    if (shmctl(goodRequestShareMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    debug("Port clean");

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    if (simulationRunning == 1) {
        kill(getppid(), SIGINT);
    }
    exit(exitNumber);
}