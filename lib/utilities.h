typedef struct coordinates Coordinates;
struct coordinates {
    double x;
    double y;
    int msgQueuId;
};

Coordinates getRandomCoordinates();

int getSharedMemory();