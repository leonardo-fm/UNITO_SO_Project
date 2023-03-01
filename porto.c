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

const int HOUR_IN_DAY = 24;

int *configArr = 0;

goodDailyDump *goodDumpArr = 0; 
portDailyDump *portDumpArr = 0; 
int *acknowledgeInitArr = 0;
int *acknowledgeDumpArr = 0;
goodEndDump *endGoodDumpArr = 0;
sem_t *endGoodDumpSemaphore = 0; 
Goods *goodMasterArr = 0;

Port *portArr = 0;
Port *port = 0;

int goodStockSharedMemoryId = 0;
Goods *goodStockArr = 0;
sem_t *goodStockSemaphore = 0;

int goodRequestSharedMemoryId = 0;
Goods *goodRequestArr = 0;
sem_t *goodRequestSemaphore = 0;

int simulationRunning = 1;
int inSwall = 0;
int masterPid = 0;

void handle_port_simulation_signals(int signal) {

    switch (signal)
    {
         case SIGUSR1: 
            printConsole("Port in SIG1");
            setAcknowledge();
            waitForSignal(SIGUSR2);
            printConsole("Port in SIG2");
            setAcknowledge();

            dumpData();

            waitForSignal(SIGUSR2);
            printConsole("Port in SIG2-2");
            setAcknowledge();

            newDay();
            break;

        case SIGPROF: /* Swell */
            printf("SWELL for id %d\n", port->id);
            handleSwell();
            break;

        case SIGSYS: /* End simulation */
            dumpData();
            simulationRunning = 0;
            stopWaitingQueues = 1;
            break;
            
        default:
            handleError("Intercept a unhandled signal");
            break;
    }

    if (inSwall == 1) {
        printf("%d recive signal %d\n",port->id, signal);
        waitForSignal(SIGPROF);
    }
}

void handle_port_stopProcess() {

    /* Block all incoming signals after the first SIGINT */
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    debug("Stopping port...");
    cleanup();
    exit(0);
}

/* argv[0]=id | argv[1]=configsh | argv[2]=portsh | argv[3]=goodsh | 
    argv[4]=ganalizersh | argv[5]=pc analyzersh | argv[6]=akish | argv[7]=endanalyzershm */
int main(int argx, char *argv[]) {

    (void) argx;
    initializeEnvironment();
    initializeSingalsMask();
    initializeSingalsHandlers();

    if (initializeConfig(argv[1], argv[4], argv[5], argv[6], argv[7], argv[8], argv[3]) == -1) {
        handleError("Initialization of port config failed");
        safeExit(1);
    }

    if (initializePort(argv[0], argv[2]) == -1) {
        handleError("Initialization of port failed");
        safeExit(2);
    }

    if (work() == -1) {
        handleError("Error during port work");
        safeExit(3);
    }

    /* Aknowledge finish */
    setAcknowledge();
    
    if (cleanup() == -1) {
        handleError("Port cleanup failed");
        safeExit(4);
    }

    return 0;
}

void initializeSingalsMask() {

    sigset_t sigMask;

    sigfillset(&sigMask);
    sigdelset(&sigMask, SIGUSR1);
    sigdelset(&sigMask, SIGPROF);
    sigdelset(&sigMask, SIGSYS);
    sigdelset(&sigMask, SIGINT);
    sigprocmask(SIG_SETMASK, &sigMask, NULL);
}

int initializeSingalsHandlers() {

    struct sigaction signalAction;

    setpgid(getpid(), getppid());
    masterPid = getppid();

    /* Use different method because i need to use the handler multiple times */
    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_port_simulation_signals;
    sigaction(SIGUSR1, &signalAction, NULL);

    signalAction.sa_flags = SA_RESTART;
    signalAction.sa_handler = &handle_port_simulation_signals;
    sigaction(SIGPROF, &signalAction, NULL);

    signal(SIGSYS, handle_port_simulation_signals);
    signal(SIGINT, handle_port_stopProcess);

    return 0;
}

int initializeConfig(char *configSharedMemoryIdString, char *goodAnalyzerSharedMemoryIdString, 
    char *portAnalyzerSharedMemoryIdString, char *acknowledgeInitSharedMemoryIdS, char *endGoodSharedMemoryIdS,
    char *acknowledgeDumpSharedMemoryIdS, char *goodMasterSharedMemoryIdS) {

    char *p;
    int configSharedMemoryId = strtol(configSharedMemoryIdString, &p, 10);
    int goodAnalyzerSharedMemoryId = strtol(goodAnalyzerSharedMemoryIdString, &p, 10);
    int portAnalyzerSharedMemoryId = strtol(portAnalyzerSharedMemoryIdString, &p, 10);
    int acknowledgeInitSharedMemoryId = strtol(acknowledgeInitSharedMemoryIdS, &p, 10);
    int endGoodSharedMemoryId = strtol(endGoodSharedMemoryIdS, &p, 10);
    int acknowledgeDumpSharedMemoryId = strtol(acknowledgeDumpSharedMemoryIdS, &p, 10);
    int goodMasterSharedMemoryId = strtol(goodMasterSharedMemoryIdS, &p, 10);
    
    configArr = (int*) shmat(configSharedMemoryId, NULL, 0);
    if (configArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    goodDumpArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    portDumpArr = (portDailyDump*) shmat(portAnalyzerSharedMemoryId, NULL, 0);
    if (portDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    acknowledgeInitArr = (int*) shmat(acknowledgeInitSharedMemoryId, NULL, 0);
    if (acknowledgeInitArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    endGoodDumpArr = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (endGoodDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    endGoodDumpSemaphore = sem_open(endGoodSharedMemoryIdS, O_EXCL, 0600, 1);
    if (endGoodDumpSemaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return -1;
    }

    acknowledgeDumpArr = (int*) shmat(acknowledgeDumpSharedMemoryId, NULL, 0);
    if (acknowledgeDumpArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    goodMasterArr = (Goods*) shmat(goodMasterSharedMemoryId, NULL, 0);
    if (goodMasterArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    return 0;
}

int initializePort(char *portIdString, char *portSharedMemoryIdS) {

    if (initializePortStruct(portIdString, portSharedMemoryIdS) == -1) {
        handleError("Error occurred during init of port struct");
        return -1;
    }

    if (initializeExchangeGoods() == -1) {
        handleError("Error occurred during init of goods exchange");
        return -1;
    }

    if (initializePortGoods() == -1) {
        handleError("Error occurred during init of goods");
        return -1;
    }

    /* Aknowledge finish */
    setAcknowledge();

    return 0;
}

int initializePortStruct(char *portIdString, char *portSharedMemoryIdS) {
        
    char *p;
    char queueKey[12];
    int portId, portMsgId, sharedMemoryId;

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

    sharedMemoryId = strtol(portSharedMemoryIdS, &p, 10);
    portArr = (Port*) shmat(sharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }
    
    portArr[portId].id = portId;
    portArr[portId].pid = getpid();
    portArr[portId].msgQueuId = portMsgId;
    if (portArr[portId].id < 4) {
        portArr[portId].position = getCornerCoordinates(configArr[SO_LATO], configArr[SO_LATO], portArr[portId].id);
    } else {
        portArr[portId].position = getRandomCoordinates(configArr[SO_LATO], configArr[SO_LATO]);
    }
    portArr[portId].quays = getRandomValue(1, configArr[SO_BANCHINE]);
    portArr[portId].availableQuays = portArr[portId].quays;

    port = &portArr[portId];

    return 0;
}

int initializeExchangeGoods() {

    int maxRequest, i;

    /* Generate shared memory for good stock */
    goodStockSharedMemoryId = generateSharedMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodStockSharedMemoryId == -1) {
        handleError("Error during creation of shared memory for goods stock");
        return -1;
    }

    goodStockArr = (Goods*) shmat(goodStockSharedMemoryId, NULL, 0);
    if (goodStockArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }

    goodStockSemaphore = generateSemaphore(goodStockSharedMemoryId);
    if (goodStockSemaphore == (void*) -1) {
        handleError("Error during creation of semaphore for goods stock");
        return -1;
    }

    /* Generate shared memory for good request */
    goodRequestSharedMemoryId = generateSharedMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodRequestSharedMemoryId == -1) {
        handleError("Error during creation of shared memory for goods request");
        return -1;
    }

    goodRequestArr = (Goods*) shmat(goodRequestSharedMemoryId, NULL, 0);
    if (goodRequestArr == (void*) -1) {
        handleErrno("shmat()");
        return -1;
    }
    
    goodRequestSemaphore = generateSemaphore(goodRequestSharedMemoryId);
    if (goodRequestSemaphore == (void*) -1) {
        handleError("Error during creation of semaphore for goods request");
        return -1;
    }

    maxRequest = (configArr[SO_FILL] / 2) / configArr[SO_MERCI] / configArr[SO_PORTI];
    for (i = 0; i < configArr[SO_MERCI]; i++) {

        Goods goodInStock, goodRequested;

        goodInStock.id = i;
        goodInStock.loadInTon = 0;
        goodInStock.state = In_The_Port;

        goodStockArr[i] = goodInStock;

        goodRequested.id = i;
        goodRequested.loadInTon = maxRequest;
        goodRequested.state = In_The_Port;

        goodRequestArr[i] = goodRequested;
    }

    return 0;
}

int initializePortGoods() {

    int i;

    for (i = 0; i < configArr[SO_MERCI]; i++) {
        
        int stockOrRequest = getRandomValue(0, 1);

        if (stockOrRequest == 0) {
            /* Fill stock */
            goodStockArr[i].remaningDays = goodMasterArr[i].remaningDays;
            goodStockArr[i].loadInTon = goodMasterArr[i].loadInTon;
            goodRequestArr[i].loadInTon = 0;
            goodStockArr[i].state = In_The_Port;
            
            sem_wait(endGoodDumpSemaphore);
            endGoodDumpArr[i].totalInitNumber += goodStockArr[i].loadInTon;
            sem_post(endGoodDumpSemaphore);
        } else {
            /* Fill request */
            goodRequestArr[i].remaningDays = goodMasterArr[i].remaningDays;
            goodRequestArr[i].loadInTon = goodMasterArr[i].loadInTon;
            goodStockArr[i].loadInTon = 0;
            goodRequestArr[i].state = In_The_Port;
        }

        goodStockArr[i].dailyExchange = 0;
        goodRequestArr[i].dailyExchange = 0;
    }

    return 0;
}

int handleSwell() {

    if (inSwall == 0) {
        port->swell++;
        inSwall = 1;
    } else {
        inSwall = 0;
    }

    return 0;
}

int work() {

    int maxQauys, i, j;
    int *queues[2];

    /* wait for simulation to start */
    if (waitForSignal(SIGUSR1) != 0) {
        handleError("Error while waiting for start");
        return -1;
    }

    /* queue[0][n] read queue | queue[1][n] write queue */
    maxQauys = port->availableQuays;
    queues[0] = (int*) malloc(sizeof(int) * maxQauys);
    queues[1] = (int*) malloc(sizeof(int) * maxQauys);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < maxQauys; j++) {
            queues[i][j] = -1;
        }
    }

    /* Remains alive until all the quays are empty to avoid boats closing errors */
    while (simulationRunning == 1 || (port->quays - port->availableQuays) > 0)
    {
        if (simulationRunning == 1) {
            PortMessage setupMsg;
            int flag = port->availableQuays == port->quays ? 0 : IPC_NOWAIT;
            int setupMsgStatus = receiveMessage(port->msgQueuId, &setupMsg, flag, 1);

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
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        sem_wait(endGoodDumpSemaphore);
        endGoodDumpArr[i].inPort += goodStockArr[i].loadInTon;
        sem_post(endGoodDumpSemaphore);
    }

    return 0;
}

int freePendingMsgs() {

    /* Check for pending requests */
    struct msqid_ds msgInfo;
    int msgInfoResponse, pendingMsg, i;
    
    msgInfoResponse = msgctl(port->msgQueuId, IPC_STAT, &msgInfo);
    if (msgInfoResponse == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    pendingMsg = (int) msgInfo.msg_qnum;  
    for (i = 0; i < pendingMsg; i++) {
        PortMessage pendingMsg;
        int pendingMsgStatus = receiveMessage(port->msgQueuId, &pendingMsg, 0, 0);

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

    goodDailyDump gdd;
    portDailyDump pdd;

    /* Init data */
    totalGoodInStock = 0;
    totalGoodRequested = 0;
    totalDailyGoodsSold = 0;
    totalDailyGoodsRecived = 0;
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(goodStockArr[i].state != Expired_In_The_Port) {
            totalGoodInStock += goodStockArr[i].loadInTon;
        }
        if(goodRequestArr[i].state != Expired_In_The_Port) {
            totalGoodRequested += goodRequestArr[i].loadInTon;
        }
        
        totalDailyGoodsSold += goodStockArr[i].dailyExchange;        
        totalDailyGoodsRecived += goodRequestArr[i].dailyExchange;
    }

    /* Send goods data */
    goodReferenceId = port->id * configArr[SO_MERCI];
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        
        gdd.goodId = i;
        if (goodStockArr[i].state == Expired_In_The_Port) {
            gdd.Good_Expired_In_The_Port = goodStockArr[i].loadInTon;
            
            goodStockArr[i].loadInTon = 0;
            goodStockArr[i].state = Undefined;
        } else {
            gdd.Good_Expired_In_The_Port = 0;
        }

        gdd.Good_In_The_Port = goodStockArr[i].loadInTon;
        gdd.Good_Delivered = goodRequestArr[i].dailyExchange;

        gdd.Good_Expired_In_The_Boat = 0;
        gdd.Good_In_The_Boat = 0;

        memcpy(&goodDumpArr[goodReferenceId + i], &gdd, sizeof(goodDailyDump)); 
    }

    /* Send port data */
    pdd.id = port->id;
    pdd.totalGoodInStock = totalGoodInStock;
    pdd.totalGoodRequested = totalGoodRequested;
    pdd.totalGoodRecived = totalDailyGoodsRecived;
    pdd.totalGoodSold = totalDailyGoodsSold;
    pdd.totalQuays = port->quays;
    pdd.busyQuays = port->quays - port->availableQuays;
    pdd.swell = port->swell;

    portDumpArr[port->id] = pdd;

    port->swell = 0;

    /* Acknowledge end of data dump */
    acknowledgeDumpArr[port->id] = 1;

    return 0;
}

int newDay() {
    
    int i, getNewGoods;
    char buffer[128];

    /* Get random value to get new goods in the port */
    getNewGoods = getRandomValue(0, 1);

    if (getNewGoods == 1) {
        
        if (initializePortGoods() == -1) {
            handleError("Error occurred during daily init of goods");
            return -1;
        }
        debug("Got new good for the port");
    } else {
        
        /* TODO nel mentre che avviene uno scambio con una nave, se la merce scade allora la nave la conta come da se e anche il porto */
        for (i = 0; i < configArr[SO_MERCI]; i++) {
            if(goodStockArr[i].remaningDays > 0) {
                goodStockArr[i].remaningDays--;
                if (goodStockArr[i].remaningDays == 0) {
                    goodStockArr[i].state = Expired_In_The_Port;

                    sem_wait(endGoodDumpSemaphore);
                    endGoodDumpArr[i].expiredInPort += goodStockArr[i].loadInTon;
                    sem_post(endGoodDumpSemaphore);
                }
            }

            /* reset daily exchanges */
            goodStockArr[i].dailyExchange = 0;
            goodRequestArr[i].dailyExchange = 0;
        }
    }

    snprintf(buffer, sizeof(buffer), "Port %d, free quays %d/%d", port->id, port->availableQuays, port->quays);
    debug(buffer);

    return 0;
}

/* If the port accept the boat request return 0, if not return 1, for errors return -1 */
int handlePA_ACCEPT(int queueId) {

    if (port->availableQuays > 0 && simulationRunning == 1) {
        if (sendMessage(queueId, PA_Y, -1, -1) == -1) {
            handleError("Error during send ACCEPT");
            return -1;
        }

        port->availableQuays--;

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
        if (sendMessage(queueId, PA_Y, goodRequestSharedMemoryId, goodRequestSharedMemoryId) == -1) {
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

    arrRequest = (Goods*) shmat(goodRequestSharedMemoryId, NULL, 0);
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
        if (sendMessage(queueId, PA_Y, goodStockSharedMemoryId, goodStockSharedMemoryId) == -1) {
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

    goodStockArr[goodId].dailyExchange += exchangeQuantity;
    
    sem_wait(endGoodDumpSemaphore);
    endGoodDumpArr[goodId].exchanged += exchangeQuantity;
    sem_post(endGoodDumpSemaphore);

    return 0;
}

int handlePA_EOT(int writeQueueId) {

    /* The port acknowledge to end the transmission */
    if (sendMessage(writeQueueId, PA_EOT, 0, 0) == -1) {
        handleError("Error during send EOT");
        return -1;
    }

    port->availableQuays++;

    return 0;
}

int generateSharedMemory(int sizeOfSegment) {
    int sharedMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (sharedMemoryId == -1) {
        handleErrno("shmget()");
        return -1;
    }

    return sharedMemoryId;
}

/* Generate a semaphore */
/* https://stackoverflow.com/questions/32205396/share-posix-semaphore-among-multiple-processes */
sem_t *generateSemaphore(int semKey) {

    char semaphoreKey[12];
    sem_t *semaphore;

    if (sprintf(semaphoreKey, "%d", semKey) == -1) {
        handleError("Error during conversion of the pid for semaphore to a string");
        return (sem_t*) -1;
    }   

    semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        handleErrno("sem_open()");
        return (sem_t*) -1;
    }

    return semaphore;
}

void setAcknowledge() {
    
    acknowledgeInitArr[port->id] = 1;
}
 
int cleanup() {

    if (configArr != 0 && shmdt(configArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (goodDumpArr != 0 && shmdt(goodDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (portDumpArr != 0 && shmdt(portDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }
     
    if (endGoodDumpArr != 0 && shmdt(endGoodDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (endGoodDumpSemaphore != 0 && sem_close(endGoodDumpSemaphore) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (port->msgQueuId != 0 && msgctl(port->msgQueuId, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    if (goodStockArr != 0 && shmdt(goodStockArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (goodStockSharedMemoryId != 0 && shmctl(goodStockSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    if (goodStockSemaphore != 0 && sem_close(goodStockSemaphore) == -1) {
        handleErrno("sem_close()");
        return -1;
    }
    
    if (goodRequestArr != 0 && shmdt(goodRequestArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (goodRequestSharedMemoryId != 0 && shmctl(goodRequestSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrno("msgctl()");
        return -1;
    }

    if (goodRequestSemaphore != 0 && sem_close(goodRequestSemaphore) == -1) {
        handleErrno("sem_close()");
        return -1;
    }

    if (acknowledgeInitArr != 0 && shmdt(acknowledgeInitArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (acknowledgeDumpArr != 0 && shmdt(acknowledgeDumpArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (goodMasterArr != 0 && shmdt(goodMasterArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    if (portArr != 0 && shmdt(portArr) == -1) {
        handleErrno("shmdt()");
        return -1;
    }

    debug("Port clean");

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    if (simulationRunning == 1 && masterPid == getppid()) {
        kill(getppid(), SIGINT);
    }
    exit(exitNumber);
}