#include <time.h>

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
    SO_DAYS 
} ConfigurationVariables;


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
    int loadInTon;
    int remaningDays;
    int dailyExchange;
    GoodsState state;
} Goods;


/* BOAT */

typedef enum { 
    In_Sea,
    In_Sea_Empty,
    In_Port_Exchange
} BoatState;

typedef struct {
    int id;
    int speed;
    int capacityInTon;
    Coordinates position;
    BoatState state;
} Boat;


/* PORT */

typedef struct {
    int id;
    int msgQueuId;
    Coordinates position;
    int quays;
    int availableQuays;
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
    BoatState boatState;
} boatDailyDump;

typedef struct {
    int id;
    int totalGoodInStock;
    int totalGoodSold;
    int totalGoodRecived;
    int busyQuays;
    int totalQuays;
} portDailyDump;