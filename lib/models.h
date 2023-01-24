/* OTHER */

typedef struct coordinates Coordinates;
struct coordinates {
    double x;
    double y;
    int msgQueuId;
};


/* GOODS */

enum goodsState { 
    Undefined,
    In_The_Port, 
    In_The_Boat, 
    Delivered, 
    Expired_In_The_Port, 
    Expired_In_The_Boat 
};
typedef enum goodsState GoodsState;

struct goods {
    int id;
    double loadInTon;
    int lifespan;
    int remaningDays;
    GoodsState state;
    char* name[12];
};
typedef struct goods Goods;

/* BOAT */

enum boatState { 
    In_Port,
    In_Sea
};
typedef enum boatState BoatState;

struct boat {
    int id;
    int speed;
    int capacityInTon;
    Coordinates position;
    BoatState state;
    Goods goods[];
};
typedef struct boat Boat;


/* PORT */

enum quayState { 
    Free,
    Busy
};
typedef enum quayState QuayState;

struct quay {
    QuayState state;
};
typedef struct quay Quay;

typedef struct port Port;
struct port {
    Coordinates position;
    /*Goods supplies[];
    Goods demands[];*/
    Quay quays[];
};