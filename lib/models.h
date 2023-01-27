#include <time.h>

/* OTHER */

typedef struct {
    double x;
    double y;
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
    GoodsState state;
} Goods;


/* BOAT */

typedef enum { 
    In_Port,
    In_Sea
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