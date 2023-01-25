/* OTHER */

typedef struct {
    double x;
    double y;
}Coordinates;


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
    double loadInTon;
    int lifespan;
    int remaningDays;
    GoodsState state;
} Goods;

typedef struct {
    int** exchange;
} GoodExchange;


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
    Goods goods[];
} Boat;


/* PORT */

typedef struct {
    int id;
    int msgQueuId;
    Coordinates position;
    int quays;
    int availableQuays;
} Port;