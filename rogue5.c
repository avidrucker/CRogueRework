#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 3
#define BIG_SIZE 30
#define SUBGRID_SIZE 10
#define MIN_ROOM_DIM 5
#define MAX_ROOM_DIM 9

int rooms[SIZE][SIZE] = {0};
int horizontal_corridors[SIZE][SIZE] = {0};
int vertical_corridors[SIZE][SIZE] = {0};

char bigMap[BIG_SIZE][BIG_SIZE];

typedef struct {
    int x, y;
    int width, height;
    int exists;
} TiledRoom;

TiledRoom tiledRooms[SIZE][SIZE];

void shuffle(int *array, size_t n) {
    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }
}

void recursiveBacktracking(int x, int y, int *room_count, int max_rooms) {
    rooms[y][x] = 1;
    (*room_count)++;

    int directions[] = {0,1,2,3}; // up,right,down,left
    shuffle(directions,4);

    for (int i = 0; i < 4; i++) {
        int nx = x, ny = y;
        switch (directions[i]) {
            case 0: ny--; break; // up
            case 1: nx++; break; // right
            case 2: ny++; break; // down
            case 3: nx--; break; // left
        }
        if (nx<0||nx>=SIZE||ny<0||ny>=SIZE) continue;
        if (rooms[ny][nx]) continue; // Already a room

        // Mark corridor always from smaller cell
        if (ny > y) { // going down
            vertical_corridors[y][x] = 1; 
        } else if (ny < y) { // going up
            vertical_corridors[ny][x] = 1; 
        } else if (nx > x) { // going right
            horizontal_corridors[y][x] = 1;
        } else if (nx < x) { // going left
            horizontal_corridors[y][nx] = 1;
        }

        recursiveBacktracking(nx, ny, room_count, max_rooms);
        if (*room_count >= max_rooms) return;
    }
}

void generateMaze() {
    srand(time(NULL));
    int room_count = 0;
    int max_rooms = 6 + rand() % 4; // Random number between 6 and 9
    int startX = rand() % SIZE;    // Random starting position
    int startY = rand() % SIZE;

    recursiveBacktracking(startX, startY, &room_count, max_rooms);
}

void printMaze() {
    // Print the larger grid
    for (int y = 0; y < SIZE; y++) {
        // Print the row with rooms and horizontal corridors
        for (int x = 0; x < SIZE; x++) {
            if (rooms[y][x]) {
                printf("R");
            } else {
                printf("#");
            }

            if (x < SIZE - 1) {
                if (horizontal_corridors[y][x] && rooms[y][x] && rooms[y][x + 1]) {
                    printf("---");
                } else {
                    printf("###");
                }
            }
        }
        printf("\n");

        // Print the row with vertical corridors
        if (y < SIZE - 1) {
            for (int x = 0; x < SIZE; x++) {
                if (vertical_corridors[y][x] && rooms[y][x] && rooms[y + 1][x]) {
                    printf("|   ");
                } else {
                    printf("#   ");
                }
            }
            printf("\n");
        }
    }
}

void clearBigMap() {
    for (int y = 0; y < BIG_SIZE; y++) {
        for (int x = 0; x < BIG_SIZE; x++) {
            bigMap[y][x] = ' ';
        }
    }
}

void positionRoomsInQuadrants() {
    int margin = 1; // Keep 1-tile border around every quadrant

    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            tiledRooms[gy][gx].exists = 0;
            if (!rooms[gy][gx]) continue; // skip if not a room

            tiledRooms[gy][gx].exists = 1;

            int w = MIN_ROOM_DIM + rand() % (MAX_ROOM_DIM - MIN_ROOM_DIM + 1);
            int h = MIN_ROOM_DIM + rand() % (MAX_ROOM_DIM - MIN_ROOM_DIM + 1);

            int quadX = gx * SUBGRID_SIZE;
            int quadY = gy * SUBGRID_SIZE;

            // Subgrid is 10×10. We want 1 tiles of padding on left/right and top/bottom
            // => effectively we only have (10 - 2) = 8 tiles for the interior max left
            int availableW = SUBGRID_SIZE - w - 2*margin; 
            int availableH = SUBGRID_SIZE - h - 2*margin; 

            if (availableW < 0) availableW = 0;  // safeguard
            if (availableH < 0) availableH = 0;

            int roomLeft = quadX + margin + rand() % (availableW + 1);
            int roomTop  = quadY + margin + rand() % (availableH + 1);

            tiledRooms[gy][gx].x = roomLeft;
            tiledRooms[gy][gx].y = roomTop;
            tiledRooms[gy][gx].width = w;
            tiledRooms[gy][gx].height = h;
        }
    }
}

void drawRoom(TiledRoom *r) {
    int left   = r->x;
    int top    = r->y;
    int right  = left + r->width - 1;
    int bottom = top + r->height - 1;

    // 1) Draw corners
    bigMap[top][left]     = '&';
    bigMap[top][right]    = '&';
    bigMap[bottom][left]  = '&';
    bigMap[bottom][right] = '&';

    // 2) Draw top/bottom edges (excluding corners)
    for (int x = left + 1; x < right; x++) {
        bigMap[top][x]    = '-';
        bigMap[bottom][x] = '-';
    }

    // 3) Draw left/right edges (excluding corners)
    for (int y = top + 1; y < bottom; y++) {
        bigMap[y][left]  = '|';
        bigMap[y][right] = '|';
    }

    // 4) Fill interior
    for (int y = top + 1; y < bottom; y++) {
        for (int x = left + 1; x < right; x++) {
            bigMap[y][x] = '.';
        }
    }
}

void drawAllRooms() {
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (tiledRooms[gy][gx].exists) {
                drawRoom(&tiledRooms[gy][gx]);
            }
        }
    }
}

void carveCorridor(int x1, int y1, int x2, int y2) {
    // Mark the starting door tile itself as corridor
    bigMap[y1][x1] = '%';

    // Decide randomly whether to move in X first or Y first
    int doXFirst = rand() % 2;

    if (x1 == x2) {
        // purely vertical
        doXFirst = 0; 
    } 
    else if (y1 == y2) {
        // purely horizontal
        doXFirst = 1; 
    }

    if (doXFirst) {
        // Move in X direction until x == x2
        while (x1 != x2) {
            x1 += (x2 > x1) ? 1 : -1;
            bigMap[y1][x1] = '%';
        }
        // Then move in Y direction
        while (y1 != y2) {
            y1 += (y2 > y1) ? 1 : -1;
            bigMap[y1][x1] = '%';
        }
    } else {
        // Move in Y direction first
        while (y1 != y2) {
            y1 += (y2 > y1) ? 1 : -1;
            bigMap[y1][x1] = '%';
        }
        // Then move in X direction
        while (x1 != x2) {
            x1 += (x2 > x1) ? 1 : -1;
            bigMap[y1][x1] = '%';
        }
    }
}

// Place '#' doors on facing walls where corridors exist
void placeDoorsForCorridors() {
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (!tiledRooms[gy][gx].exists) continue;

            TiledRoom *r1 = &tiledRooms[gy][gx];

            // Horizontal corridor to (gx+1, gy)
            if (gx < SIZE - 1 && horizontal_corridors[gy][gx] == 1) {
                TiledRoom *r2 = &tiledRooms[gy][gx + 1];
                if (!r2->exists) continue;

                int left1   = r1->x;
                int top1    = r1->y;
                int right1  = left1 + r1->width - 1;
                int bottom1 = top1 + r1->height - 1;

                int left2   = r2->x;
                int top2    = r2->y;
                int right2  = left2 + r2->width - 1;
                int bottom2 = top2 + r2->height - 1;

                // x-coords are the walls
                int doorX1 = right1;
                int doorX2 = left2;

                if (r1->height > 2 && r2->height > 2) {
                    int doorY1 = top1 + 1 + rand() % (r1->height - 2);
                    int doorY2 = top2 + 1 + rand() % (r2->height - 2);

                    // place the door tiles
                    bigMap[doorY1][doorX1] = '#';  // east wall of R1
                    bigMap[doorY2][doorX2] = '#';  // west wall of R2

                    // Now carve corridor from (doorX1, doorY1) to (doorX2, doorY2)
                    carveCorridor(doorX1+1, doorY1, doorX2-1, doorY2);
                }
            }            

            // Vertical corridor to (gx, gy+1)
            if (gy < SIZE - 1 && vertical_corridors[gy][gx] == 1) {
                TiledRoom *r2 = &tiledRooms[gy + 1][gx];
                if (!r2->exists) continue;

                int left1   = r1->x;
                int top1    = r1->y;
                int right1  = left1 + r1->width - 1;
                int bottom1 = top1 + r1->height - 1;

                int left2   = r2->x;
                int top2    = r2->y;
                int right2  = left2 + r2->width - 1;
                int bottom2 = top2 + r2->height - 1;

                // x-coords are the walls
                int doorY1 = bottom1;
                int doorY2 = top2;

                // printf("  Room1 bottom1=%d, top1=%d, left1=%d, right1=%d\n", bottom1, top1, left1, right1);
                // printf("  Room2 top2=%d, left2=%d, right2=%d\n", top2, left2, right2);

                if (r1->width > 2 && r2->width > 2) {
                    int doorX1 = left1 + 1 + rand() % (r1->width - 2);
                    int doorX2 = left2 + 1 + rand() % (r2->width - 2);
                    
                    // printf("  chosen doorX1=%d, doorX2=%d\n", doorX1, doorX2);

                    bigMap[doorY1][doorX1] = '#'; // bottom wall of r1
                    bigMap[doorY2][doorX2]    = '#'; // top wall of r2

                    // Now carve corridor from (doorX1, doorY1) to (doorX2, doorY2)
                    carveCorridor(doorX1, doorY1+1, doorX2, doorY2-1);
                }
            }
        }
    }
}

void printBigMap() {
    for (int y = 0; y < BIG_SIZE; y++) {
        for (int x = 0; x < BIG_SIZE; x++) {
	    // print a character + a space
            printf("%c ", bigMap[y][x]);
        }
        printf("\n");
    }
}

int main() {
    srand(time(NULL));
    generateMaze();  // Creates 3x3 map with up to 6-9 rooms
    printMaze();     // (Optional) See the "R---R" debug output

    // Now generate the 90x90 tiled map:
    clearBigMap();
    positionRoomsInQuadrants();
    drawAllRooms();
    placeDoorsForCorridors();
    printBigMap();

    return 0;
}

