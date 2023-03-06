/* OTHER */

typedef struct {
    int x;
    int y;
} Coordinates;

typedef enum {
    SO_NAVI,
    SO_PORTI,
    SO_MERCI,
    SO_SIZE,
    SO_MIN_VITA,
    SO_MAX_VITA,
    SO_LATO,
    SO_SPEED,
    S0_CAPACITY,
    SO_BANCHINE,
    SO_FILL,
    SO_LOADSPEED,
    SO_DAYS,
    SO_STORM_DURATION,
    SO_SWELL_DURATION,
    SO_MALESTORM
} ConfigurationVariables;

typedef enum { 
    Es_Initializing,
    Es_Running,
    Es_Stopped,
    Es_Waiting_Start,
    Es_Waiting_Dump,
    Es_Waiting_Continue,
    Es_Finish_Simulation,
    Es_Finish_Execution
} ExecutionStates;

/* GOODS */

typedef enum { 
    Undefined,
    In_The_Port, 
    In_The_Boat, 
    Delivered, 
    Expired_In_The_Port, 
    Expired_In_The_Boat 
} GoodsState;

typedef struct {
    int id;
    int goodLots;
    int remaningDays;
    int dailyExchange;
    GoodsState state;
} Goods;


/* BOAT */

typedef enum { 
    In_Sea,
    In_Sea_Travelling,
    In_Sea_Empty,
    In_Sea_Empty_Travelling,
    In_Port_Exchange,
    Sunk
} BoatState;

typedef struct {
    int id;
    int pid;
    int capacityInLot;
    int storm;
    Coordinates position;
    BoatState state;
} Boat;


/* PORT */

typedef enum { 
    Operative,
    In_Swell
} PortState;

typedef struct {
    int id;
    int pid;
    int msgQueuId;
    int quays;
    int availableQuays;
    int swell;
    PortState state;
    Coordinates position;
} Port;


/* ANALYZER */

typedef struct {
    int goodId;
    int Good_In_The_Port;
    int Good_In_The_Boat;
    int Good_Delivered;
    int Good_Expired_In_The_Port;
    int Good_Expired_In_The_Boat;
} goodDailyDump;

typedef struct {
    int id;
    int storm;
    int malestorm;
    BoatState boatState;
} boatDailyDump;

typedef struct {
    int id;
    int totalGoodInStock;
    int totalGoodRequested;
    int totalGoodSold;
    int totalGoodRecived;
    int busyQuays;
    int totalQuays;
    int swell;
} portDailyDump;

typedef struct {
    int totalLotInitNumber;
    int inPort;
    int expiredInPort;
    int expiredInBoat;
    int exchanged;
} goodEndDump;

/* WEATEHR */

typedef struct {
    int pid;
    int remaningHour;
    int signalToSend;
} weatherStatusHandler;