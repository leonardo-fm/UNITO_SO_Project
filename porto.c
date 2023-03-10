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
#include "lib/customMacro.h"

#include "lib/porto.h"

const int HOUR_IN_DAY = 24;

int *configArr = 0;

goodDailyDump *goodDumpArr = 0; 
portDailyDump *portDumpArr = 0; 
int *acknowledgeInitArr = 0;
int *acknowledgeDumpArr = 0;
goodEndDump *endGoodsDumpArr = 0;
sem_t *endGoodsDumpSemaphore = 0; 

InitGoods *goodsMasterArr = 0;
sem_t *goodsMasterSemaphore = 0; 

Port *portArr = 0;
Port *port = 0;

int goodStockSharedMemoryId = 0;
Goods *goodStockArr = 0;
sem_t *goodStockSemaphore = 0;

int goodRequestSharedMemoryId = 0;
Goods *goodRequestArr = 0;
sem_t *goodRequestSemaphore = 0;

int inSwell = 0;
int masterPid = 0;
ExecutionStates status = Es_Initializing;

void handle_port_simulation_signals(int signal) {
    
    switch (signal)
    {
         case SIGUSR1:
            switch (status)
            {
                case Es_Running:
                    setAcknowledge();
                    status = Es_Waiting_Dump;
                    raise(SIGSTOP);
                    break;
                case Es_Waiting_Dump:
                    dumpData();
                    setAcknowledge();
                    status = Es_Waiting_Continue;
                    raise(SIGSTOP);
                    break;
                case Es_Waiting_Continue:
                    setAcknowledge();
                    status = Es_Running;
                    newDay();
                    break;                
                default:
                    handleErrorId("Recived not handled status", port->id);
                    break;
            }
            break;

        case SIGPROF: /* Swell */
            debugId("Start swell", port->id);
            handleSwell();
            break;

        case SIGUSR2: /* End simulation */
            dumpData();
            status = Es_Finish_Simulation;
            stopWaitingQueues = 1;
            simulationFinished = 1; /* For the nanosleep */
            break;
            
        default:
            handleErrorId("Intercept a unhandled signal", port->id);
            break;
    }
}

void handle_port_stopProcess() {

    /* Block all incoming signals after the first SIGINT */
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    debugId("Stopping port...", port->id);
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
        handleErrorId("Initialization of port config failed", port->id);
        safeExit(1);
    }

    if (initializePort(argv[0], argv[2]) == -1) {
        handleErrorId("Initialization of port failed", port->id);
        safeExit(2);
    }

    if (work() == -1) {
        handleErrorId("Error during port work", port->id);
        safeExit(3);
    }

    /* Aknowledge finish */
    debugId("Port finish work", port->id);
    status = Es_Finish_Execution;
    setAcknowledge();
    
    if (cleanup() == -1) {
        handleErrorId("Port cleanup failed", port->id);
        safeExit(4);
    }

    return 0;
}

void initializeSingalsMask() {

    sigset_t sigMask;

    sigfillset(&sigMask);
    sigdelset(&sigMask, SIGUSR1);
    sigdelset(&sigMask, SIGUSR2);

    sigdelset(&sigMask, SIGPROF);
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

    signal(SIGUSR2, handle_port_simulation_signals);
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
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    goodDumpArr = (goodDailyDump*) shmat(goodAnalyzerSharedMemoryId, NULL, 0);
    if (goodDumpArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    portDumpArr = (portDailyDump*) shmat(portAnalyzerSharedMemoryId, NULL, 0);
    if (portDumpArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    acknowledgeInitArr = (int*) shmat(acknowledgeInitSharedMemoryId, NULL, 0);
    if (acknowledgeInitArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    endGoodsDumpArr = (goodEndDump*) shmat(endGoodSharedMemoryId, NULL, 0);
    if (endGoodsDumpArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    endGoodsDumpSemaphore = sem_open(endGoodSharedMemoryIdS, O_EXCL, 0600, 1);
    if (endGoodsDumpSemaphore == SEM_FAILED) {
        handleErrnoId("sem_open()", port->id);
        return -1;
    }

    acknowledgeDumpArr = (int*) shmat(acknowledgeDumpSharedMemoryId, NULL, 0);
    if (acknowledgeDumpArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    goodsMasterArr = (InitGoods*) shmat(goodMasterSharedMemoryId, NULL, 0);
    if (goodsMasterArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    goodsMasterSemaphore = sem_open(goodMasterSharedMemoryIdS, O_EXCL, 0600, 1);
    if (endGoodsDumpSemaphore == SEM_FAILED) {
        handleErrnoId("sem_open()", port->id);
        return -1;
    }

    return 0;
}

int initializePort(char *portIdString, char *portSharedMemoryIdS) {

    if (initializePortStruct(portIdString, portSharedMemoryIdS) == -1) {
        handleErrorId("Error occurred during init of port struct", port->id);
        return -1;
    }

    if (initializeExchangeGoods() == -1) {
        handleErrorId("Error occurred during init of goods exchange", port->id);
        return -1;
    }

    if (getPortGoods() == -1) {
        handleErrorId("Error occurred during init of goods", port->id);
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
        handleErrorId("Error during conversion of the port id to a string", port->id);
        return -1;
    }

    portMsgId = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (portMsgId == -1) {
        handleErrnoId("msgget()", port->id);
        return -1;
    }

    sharedMemoryId = strtol(portSharedMemoryIdS, &p, 10);
    portArr = (Port*) shmat(sharedMemoryId, NULL, 0);
    if (portArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }
    
    portArr[portId].id = portId;
    portArr[portId].pid = getpid();
    portArr[portId].msgQueuId = portMsgId;
    portArr[portId].state = Operative;
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

    int i;

    /* Generate shared memory for good stock */
    goodStockSharedMemoryId = generateSharedMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodStockSharedMemoryId == -1) {
        handleErrorId("Error during creation of shared memory for goods stock", port->id);
        return -1;
    }

    goodStockArr = (Goods*) shmat(goodStockSharedMemoryId, NULL, 0);
    if (goodStockArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }

    goodStockSemaphore = generateSemaphore(goodStockSharedMemoryId);
    if (goodStockSemaphore == (void*) -1) {
        handleErrorId("Error during creation of semaphore for goods stock", port->id);
        return -1;
    }

    /* Generate shared memory for good request */
    goodRequestSharedMemoryId = generateSharedMemory(sizeof(Goods) * configArr[SO_MERCI]);
    if (goodRequestSharedMemoryId == -1) {
        handleErrorId("Error during creation of shared memory for goods request", port->id);
        return -1;
    }

    goodRequestArr = (Goods*) shmat(goodRequestSharedMemoryId, NULL, 0);
    if (goodRequestArr == (void*) -1) {
        handleErrnoId("shmat()", port->id);
        return -1;
    }
    
    goodRequestSemaphore = generateSemaphore(goodRequestSharedMemoryId);
    if (goodRequestSemaphore == (void*) -1) {
        handleErrorId("Error during creation of semaphore for goods request", port->id);
        return -1;
    }

    for (i = 0; i < configArr[SO_MERCI]; i++) {

        Goods goodInStock, goodRequested;

        goodInStock.id = i;
        goodInStock.goodLots = 0;
        goodInStock.state = In_The_Port;

        goodStockArr[i] = goodInStock;

        goodRequested.id = i;
        goodRequested.goodLots = 0;
        goodRequested.state = In_The_Port;

        goodRequestArr[i] = goodRequested;
    }

    return 0;
}

int getPortGoods() {

    int i, gotGoods = 0;
    
    sem_wait(goodsMasterSemaphore);
    
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        
        int stockOrRequest = getRandomValue(0, 1);
        int lotsToAdd;

        if (goodsMasterArr[i].defaultLotAmmount == 0 && goodsMasterArr[i].spareLotAmmount == 0) {
            continue;
        }

        gotGoods = 1;

        /* Dump overall data */
        sem_wait(endGoodsDumpSemaphore);

        if (goodsMasterArr[i].defaultLotAmmount > 0) {
    
            lotsToAdd = goodsMasterArr[i].defaultLot;
            goodsMasterArr[i].defaultLotAmmount--;

            endGoodsDumpArr[i].totalLotNumber += goodsMasterArr[i].defaultLot;
        } else {
    
            lotsToAdd = goodsMasterArr[i].spareLot;
            goodsMasterArr[i].spareLotAmmount--;

            endGoodsDumpArr[i].totalLotNumber += goodsMasterArr[i].spareLot;
        }

        if (stockOrRequest == 0) {
            /* Fill stock */
            goodStockArr[i].remaningDays = goodsMasterArr[i].good.remaningDays;
            goodStockArr[i].goodLots = lotsToAdd;
            goodRequestArr[i].goodLots = 0;
            goodStockArr[i].state = In_The_Port;

            endGoodsDumpArr[i].inPort += goodStockArr[i].goodLots;
        } else {
            /* Fill request */
            goodRequestArr[i].remaningDays = goodsMasterArr[i].good.remaningDays;
            goodRequestArr[i].goodLots = lotsToAdd;
            goodStockArr[i].goodLots = 0;
            goodRequestArr[i].state = In_The_Port;
        }

        goodStockArr[i].dailyExchange = 0;
        goodRequestArr[i].dailyExchange = 0;

        sem_post(endGoodsDumpSemaphore);
    }

    sem_post(goodsMasterSemaphore);

    if (gotGoods == 1) {
        debugId("Got goods for the port", port->id);
    }

    return 0;
}

int handleSwell() {

    port->swell++;
    port->state = In_Swell;
    inSwell = 1;

    return 0;
}

int waitForSwell() {
    
    double swellTime = (double) 1 / HOUR_IN_DAY * configArr[SO_SWELL_DURATION];
    double waitTimeNs = getNanoSeconds(swellTime);
    double waitTimeS = getSeconds(swellTime);

    addingTime.tv_sec = waitTimeS;
    addingTime.tv_nsec = waitTimeNs;

    if (safeWait(waitTimeS, waitTimeNs) == -1) {
        handleErrorId("Error while waiting the swell", port->id);
        return -1;
    }

    inSwell = 0;
    port->state = Operative;

    return 0;
}

int work() {

    int maxQauys, i, j;
    int *queues[2];

    /* wait for simulation to start */
    status = Es_Waiting_Start;
    raise(SIGSTOP);

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
    status = Es_Running;
    while (status != Es_Finish_Simulation || (port->quays - port->availableQuays) > 0)
    {
        if (inSwell == 1) {
            if (waitForSwell() == -1) {
                handleErrorId("Error during waiting swell", port->id);
                return -1;
            }
            debugId("Finish swell", port->id);
        }

        if (status != Es_Finish_Simulation) {
            PortMessage setupMsg;
            int flag = port->availableQuays == port->quays ? 0 : IPC_NOWAIT;
            int setupMsgStatus = receiveMessage(port->msgQueuId, &setupMsg, flag, 1);

            if (setupMsgStatus == -1) {
                handleErrorId("Error during reciving message from boat", port->id);
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

        for (i = 0; i < maxQauys; i++) {

            int readingMsgQueue, writingMsgQueue, msgStatus;
            PortMessage receivedMsg;

            if (queues[0][i] == -1) {
                continue;
            }

            readingMsgQueue = queues[0][i];
            writingMsgQueue = queues[1][i];

            msgStatus = receiveMessage(readingMsgQueue, &receivedMsg, 0, 0);

            if (msgStatus == -1) {
                handleErrorId("Error during reciving message from boat", port->id);
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
                            handleErrorId("Error during ACCEPT handling", port->id);
                            return -1;
                        };
                        if (acceptResponse == 1) {
                            queues[0][i] = -1;
                            queues[1][i] = -1;
                        }
                        break;
                    case PA_SE_GOOD:
                        if (handlePA_SE_GOOD(writingMsgQueue) == -1) {
                            handleErrorId("Error during SE_GOOD handling", port->id);
                            return -1;
                        };
                        break;
                    case PA_SE_SUMMARY:
                        if (handlePA_SE_SUMMARY(receivedMsg.msg.data.data1, receivedMsg.msg.data.data2) == -1) {
                            handleErrorId("Error during PA_SE_SUMMARY handling", port->id);
                            return -1;
                        };
                        break;
                    case PA_RQ_GOOD:
                        if (handlePA_RQ_GOOD(writingMsgQueue) == -1) {
                            handleErrorId("Error during RQ_GOOD handling", port->id);
                            return -1;
                        };
                        break;
                    case PA_RQ_SUMMARY:
                        if (handlePA_RQ_SUMMARY(receivedMsg.msg.data.data1, receivedMsg.msg.data.data2) == -1) {
                            handleErrorId("Error during PA_RQ_SUMMARY handling", port->id);
                            return -1;
                        };
                        break;
                    case PA_EOT:
                        if (handlePA_EOT(writingMsgQueue) == -1) {
                            handleErrorId("Error during EOT handling", port->id);
                            return -1;
                        };
                        queues[0][i] = -1;
                        queues[1][i] = -1;
                        break;
                    default:
                        break;
                }
            } 
        }
    }

    if (freePendingMsgs() == -1) {
        handleErrorId("Error while freeing pending messages", port->id);
        return -1;
    }

    free(queues[0]);
    free(queues[1]);

    return 0;
}

int freePendingMsgs() {

    /* Check for pending requests */
    struct msqid_ds msgInfo;
    int msgInfoResponse, pendingMsg, i;
    
    msgInfoResponse = msgctl(port->msgQueuId, IPC_STAT, &msgInfo);
    if (msgInfoResponse == -1) {
        handleErrnoId("msgctl()", port->id);
        return -1;
    }

    pendingMsg = (int) msgInfo.msg_qnum;  
    for (i = 0; i < pendingMsg; i++) {
        PortMessage pendingMsg;
        int pendingMsgStatus = receiveMessage(port->msgQueuId, &pendingMsg, 0, 0);

        if (pendingMsgStatus == -1) {
            handleErrorId("Error during reciving message from boat on pending messages", port->id);
            return -1;
        }
        
        if (sendMessage(pendingMsg.msg.data.data2, PA_N, -1, -1) == -1) {
            handleErrorId("Error during send NO", port->id);
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
            totalGoodInStock += goodStockArr[i].goodLots;
        }
        if(goodRequestArr[i].state != Expired_In_The_Port) {
            totalGoodRequested += goodRequestArr[i].goodLots;
        }
        
        totalDailyGoodsSold += goodStockArr[i].dailyExchange;        
        totalDailyGoodsRecived += goodRequestArr[i].dailyExchange;
    }

    /* Send goods data */
    goodReferenceId = port->id * configArr[SO_MERCI];
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        
        gdd.goodId = i;
        if (goodStockArr[i].state == Expired_In_The_Port) {
            gdd.Good_Expired_In_The_Port = goodStockArr[i].goodLots;
            
            goodStockArr[i].goodLots = 0;
            goodStockArr[i].state = Undefined;
        } else {
            gdd.Good_Expired_In_The_Port = 0;
        }

        gdd.Good_In_The_Port = goodStockArr[i].goodLots;
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
    
    int i;
        
    if (getPortGoods() == -1) {
        handleErrorId("Error occurred during daily init of goods", port->id);
        return -1;
    }

    /* TODO nel mentre che avviene uno scambio con una nave, se la merce scade allora la nave la conta come da se e anche il porto */
    for (i = 0; i < configArr[SO_MERCI]; i++) {
        if(goodStockArr[i].remaningDays > 0) {
            goodStockArr[i].remaningDays--;
            if (goodStockArr[i].remaningDays == 0) {
                goodStockArr[i].state = Expired_In_The_Port;

                sem_wait(endGoodsDumpSemaphore);
                endGoodsDumpArr[i].expiredInPort += goodStockArr[i].goodLots;
                sem_post(endGoodsDumpSemaphore);
            }
        }

        /* reset daily exchanges */
        goodStockArr[i].dailyExchange = 0;
        goodRequestArr[i].dailyExchange = 0;
    }

    return 0;
}

/* If the port accept the boat request return 0, if not return 1, for errors return -1 */
int handlePA_ACCEPT(int queueId) {

    if (port->availableQuays > 0 && status != Es_Finish_Simulation) {
        if (sendMessage(queueId, PA_Y, -1, -1) == -1) {
            handleErrorId("Error during send ACCEPT", port->id);
            return -1;
        }

        port->availableQuays--;

    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            handleErrorId("Error during send ACCEPT", port->id);
            return -1;
        }

        return 1;
    }

    return 0;
}

int handlePA_SE_GOOD(int queueId) {

    if (status != Es_Finish_Simulation) {
        /* The boat want to sell some goods */
        if (sendMessage(queueId, PA_Y, goodRequestSharedMemoryId, goodRequestSharedMemoryId) == -1) {
            handleErrorId("Error during send SE_GOOD", port->id);
            return -1;
        }
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            handleErrorId("Error during send SE NO", port->id);
            return -1;
        }
    }
    
    return 0;
}

int handlePA_SE_SUMMARY(int goodId, int exchangeQuantity) {

    goodRequestArr[goodId].dailyExchange += exchangeQuantity;

    return 0;
}

int handlePA_RQ_GOOD(int queueId) {

    if (status != Es_Finish_Simulation) {
        /* The boat want to buy some goods */
        if (sendMessage(queueId, PA_Y, goodStockSharedMemoryId, goodStockSharedMemoryId) == -1) {
            handleErrorId("Error during send RQ_GOOD", port->id);
            return -1;
        }
    } else {
        if (sendMessage(queueId, PA_N, -1, -1) == -1) {
            handleErrorId("Error during send RQ NO", port->id);
            return -1;
        }
    }

    return 0;
}

int handlePA_RQ_SUMMARY(int goodId, int exchangeQuantity) {

    goodStockArr[goodId].dailyExchange += exchangeQuantity;
    
    sem_wait(endGoodsDumpSemaphore);
    endGoodsDumpArr[goodId].exchanged += exchangeQuantity;
    sem_post(endGoodsDumpSemaphore);

    return 0;
}

int handlePA_EOT(int writeQueueId) {

    /* The port acknowledge to end the transmission */
    if (sendMessage(writeQueueId, PA_EOT, 0, 0) == -1) {
        handleErrorId("Error during send EOT", port->id);
        return -1;
    }

    port->availableQuays++;

    return 0;
}

int generateSharedMemory(int sizeOfSegment) {
    int sharedMemoryId = shmget(IPC_PRIVATE, sizeOfSegment, 0600);
    if (sharedMemoryId == -1) {
        handleErrnoId("shmget()", port->id);
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
        handleErrorId("Error during conversion of the pid for semaphore to a string", port->id);
        return (sem_t*) -1;
    }   

    semaphore = sem_open(semaphoreKey, O_CREAT, 0600, 1);
    if (semaphore == SEM_FAILED) {
        handleErrnoId("sem_open()", port->id);
        return (sem_t*) -1;
    }

    return semaphore;
}

void setAcknowledge() {
    
    acknowledgeInitArr[port->id] = 1;
}
 
int cleanup() {

    if (configArr != 0 && shmdt(configArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (goodDumpArr != 0 && shmdt(goodDumpArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (portDumpArr != 0 && shmdt(portDumpArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }
     
    if (endGoodsDumpArr != 0 && shmdt(endGoodsDumpArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (endGoodsDumpSemaphore != 0 && sem_close(endGoodsDumpSemaphore) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (port->msgQueuId != 0 && msgctl(port->msgQueuId, IPC_RMID, NULL) == -1) {
        handleErrnoId("msgctl()", port->id);
        return -1;
    }

    if (goodStockArr != 0 && shmdt(goodStockArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (goodStockSharedMemoryId != 0 && shmctl(goodStockSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrnoId("msgctl()", port->id);
        return -1;
    }

    if (goodStockSemaphore != 0 && sem_close(goodStockSemaphore) == -1) {
        handleErrnoId("sem_close()", port->id);
        return -1;
    }
    
    if (goodRequestArr != 0 && shmdt(goodRequestArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (goodRequestSharedMemoryId != 0 && shmctl(goodRequestSharedMemoryId, IPC_RMID, NULL) == -1) {
        handleErrnoId("msgctl()", port->id);
        return -1;
    }

    if (goodRequestSemaphore != 0 && sem_close(goodRequestSemaphore) == -1) {
        handleErrnoId("sem_close()", port->id);
        return -1;
    }

    if (acknowledgeInitArr != 0 && shmdt(acknowledgeInitArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (acknowledgeDumpArr != 0 && shmdt(acknowledgeDumpArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (goodsMasterArr != 0 && shmdt(goodsMasterArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    if (portArr != 0 && shmdt(portArr) == -1) {
        handleErrnoId("shmdt()", port->id);
        return -1;
    }

    debugId("Port clean", port->id);

    return 0;
}

void safeExit(int exitNumber) {

    cleanup();
    if (status != Es_Finish_Simulation && masterPid == getppid()) {
        kill(getppid(), SIGINT);
    }
    exit(exitNumber);
}