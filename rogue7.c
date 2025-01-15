#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <curses.h>   // or <ncurses.h> depending on your platform
#include <unistd.h>   // for usleep() if you want a small delay
#include <locale.h>

/*
 * ------------------------------------------------------------
 * Constants and Global Data
 * ------------------------------------------------------------
 */

#define SIZE 3           // The 3x3 “macro” dungeon layout
#define BIG_SIZE 30      // The 30x30 “tiled” map dimensions
#define SUBGRID_SIZE 10  // Each 3x3 cell corresponds to a 10x10 subgrid
#define MIN_ROOM_DIM 5
#define MAX_ROOM_DIM 9

int playerX, playerY;       // player's position in bigMap
int hasTreasure = 0;        // 0 = not yet, 1 = got treasure
int gameRunning = 1;        // 1 = running, 0 = user quit or reached exit

// These store which 3x3 cells actually have rooms (1) and which corridors connect them
int rooms[SIZE][SIZE] = {0};
int horizontal_corridors[SIZE][SIZE] = {0};
int vertical_corridors[SIZE][SIZE]   = {0};

int dist[SIZE][SIZE]; // store BFS distances

// The big 30x30 tile map; each cell is a short string for box-drawing or filler
char bigMap[BIG_SIZE][BIG_SIZE][4];

/*
 * A TiledRoom describes a room's position in the bigMap plus its width, height, and existence.
 */
typedef struct {
    int x, y;       // top-left position in bigMap
    int width, height;
    int exists;     // 1 if there is a room, 0 if none
} TiledRoom;

// Store 3x3 of these TiledRooms, paralleling the rooms[][] array
TiledRoom tiledRooms[SIZE][SIZE];

/*
 * ------------------------------------------------------------
 * Utility and Shuffle
 * ------------------------------------------------------------
 */

/**
 * shuffle: Fisher–Yates shuffle for an int array
 * @param array The array to shuffle
 * @param n     Number of elements
 */
static void shuffle(int *array, size_t n)
{
    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }
}

/**
 * setCell: Safely place up to 3 characters plus a null terminator
 * into the bigMap cell at (x,y).
 */
static void setCell(int x, int y, const char *s)
{
    strncpy(bigMap[y][x], s, 3);
    bigMap[y][x][3] = '\0';
}

/*
 * ------------------------------------------------------------
 * Maze Generation (3x3 macro layout)
 * ------------------------------------------------------------
 */

/**
 * markCorridorBetween: Sets the appropriate horizontal/vertical corridor flags
 * for adjacency between (x,y) and (nx,ny).
 */
static void markCorridorBetween(int x, int y, int nx, int ny)
{
    // If we are moving vertically
    if (nx == x) {
        // If ny > y => going down; else going up
        if (ny > y) {
            vertical_corridors[y][x] = 1;
        } else {
            vertical_corridors[ny][x] = 1; 
        }
    }
    // Otherwise, we are moving horizontally
    else if (ny == y) {
        // If nx > x => going right; else going left
        if (nx > x) {
            horizontal_corridors[y][x] = 1;
        } else {
            horizontal_corridors[y][nx] = 1;
        }
    }
}

/**
 * recursiveBacktracking: DFS to fill the 3x3 grid with up to max_rooms rooms.
 * Starts from (x,y). Randomly picks directions to explore, sets corridor flags,
 * and recurses.
 */
static void recursiveBacktracking(int x, int y, int *room_count, int max_rooms)
{
    if (*room_count >= max_rooms) return;

    // Mark the current cell as a room
    rooms[y][x] = 1;
    (*room_count)++;

    // Shuffle directions [0=up,1=right,2=down,3=left]
    int directions[] = {0, 1, 2, 3};
    shuffle(directions, 4);

    // Explore each direction in shuffled order
    for (int i = 0; i < 4; i++) {
        int nx = x, ny = y;
        switch (directions[i]) {
            case 0: ny--; break; // up
            case 1: nx++; break; // right
            case 2: ny++; break; // down
            case 3: nx--; break; // left
        }

        // Check bounds
        if (nx < 0 || nx >= SIZE || ny < 0 || ny >= SIZE) 
            continue;

        // If already a room, skip
        if (rooms[ny][nx]) 
            continue;

        // Mark the corridor in the appropriate array
        markCorridorBetween(x, y, nx, ny);

        // Recurse to create a room in that direction
        recursiveBacktracking(nx, ny, room_count, max_rooms);
        if (*room_count >= max_rooms) 
            return;
    }
}

/**
 * generateMaze: Picks a random start cell and a random number (6-9) of total rooms,
 * then calls recursiveBacktracking() to fill in the 3x3 grid.
 */
void generateMaze()
{
    srand((unsigned)time(NULL));
    int room_count = 0;
    int max_rooms = 6 + rand() % 4; // between 6 and 9
    // printf("Generating up to %d rooms...\n", max_rooms);
    int startX    = rand() % SIZE;
    int startY    = rand() % SIZE;

    recursiveBacktracking(startX, startY, &room_count, max_rooms);
}

/**
 * printMaze: (Optional) debug print for the 3x3 macro grid with corridors.
 * Shows 'R' for a room, '#' for no room, and draws '---' / '|' for corridors.
 */
void printMaze()
{
    for (int y = 0; y < SIZE; y++) {
        // Top row: rooms plus horizontal corridors
        for (int x = 0; x < SIZE; x++) {
            printf("%c", rooms[y][x] ? 'R' : '#');

            if (x < SIZE - 1) {
                if (horizontal_corridors[y][x] && rooms[y][x] && rooms[y][x + 1]) {
                    printf("---");
                } else {
                    printf("###");
                }
            }
        }
        printf("\n");

        // Second row: vertical corridors
        if (y < SIZE - 1) {
            for (int x = 0; x < SIZE; x++) {
                if (vertical_corridors[y][x] && rooms[y][x] && rooms[y+1][x]) {
                    printf("|   ");
                } else {
                    printf("#   ");
                }
            }
            printf("\n");
        }
    }
}

/*
 * ------------------------------------------------------------
 * 30x30 Tiled Map Construction
 * ------------------------------------------------------------
 */

/**
 * clearBigMap: Fills the entire 30x30 bigMap with a single blank space.
 */
void clearBigMap()
{
    for (int y = 0; y < BIG_SIZE; y++) {
        for (int x = 0; x < BIG_SIZE; x++) {
            strcpy(bigMap[y][x], " ");
        }
    }
}

/**
 * createTiledRoom: Helper to fill a TiledRoom struct with random size and position
 * within its 10x10 subgrid, leaving a 1-tile margin. 
 */
static void createTiledRoom(TiledRoom *r, int gx, int gy)
{
    r->exists = 1;

    // Pick random width/height within [MIN_ROOM_DIM, MAX_ROOM_DIM]
    int w = MIN_ROOM_DIM + rand() % (MAX_ROOM_DIM - MIN_ROOM_DIM + 1);
    int h = MIN_ROOM_DIM + rand() % (MAX_ROOM_DIM - MIN_ROOM_DIM + 1);

    int margin = 1;
    int quadX  = gx * SUBGRID_SIZE;
    int quadY  = gy * SUBGRID_SIZE;

    int availableW = SUBGRID_SIZE - w - 2 * margin;
    int availableH = SUBGRID_SIZE - h - 2 * margin;
    if (availableW < 0) availableW = 0;
    if (availableH < 0) availableH = 0;

    // Random left/top within the quadrant
    int roomLeft = quadX + margin + rand() % (availableW + 1);
    int roomTop  = quadY + margin + rand() % (availableH + 1);

    r->x      = roomLeft;
    r->y      = roomTop;
    r->width  = w;
    r->height = h;
}

/**
 * positionRoomsInQuadrants: For every 3x3 cell marked as existing (rooms[y][x] == 1),
 * create a random TiledRoom within that cell’s corresponding subgrid in bigMap.
 */
void positionRoomsInQuadrants()
{
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            // Default to no room
            tiledRooms[gy][gx].exists = 0;

            // If the 3x3 cell is not a room, skip
            if (!rooms[gy][gx]) 
                continue;

            // Otherwise, create the TiledRoom
            createTiledRoom(&tiledRooms[gy][gx], gx, gy);
        }
    }
}

/**
 * drawRoom: Uses box-drawing characters to draw the perimeter of a single TiledRoom
 * and fills the interior with "." 
 */
void drawRoom(const TiledRoom *r)
{
    int left   = r->x;
    int top    = r->y;
    int right  = left + r->width - 1;
    int bottom = top + r->height - 1;

    // Corners
    strcpy(bigMap[top][left],     "┌");
    strcpy(bigMap[top][right],    "┐");
    strcpy(bigMap[bottom][left],  "└");
    strcpy(bigMap[bottom][right], "┘");

    // Top/bottom edges
    for (int x = left + 1; x < right; x++) {
        strcpy(bigMap[top][x],    "─");
        strcpy(bigMap[bottom][x], "─");
    }

    // Left/right edges
    for (int y = top + 1; y < bottom; y++) {
        strcpy(bigMap[y][left],  "│");
        strcpy(bigMap[y][right], "│");
    }

    // Fill interior
    for (int y = top + 1; y < bottom; y++) {
        for (int x = left + 1; x < right; x++) {
            strcpy(bigMap[y][x], ".");
        }
    }
}

/**
 * drawAllRooms: Iterates over all TiledRooms and calls drawRoom() on each that exists.
 */
void drawAllRooms()
{
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (tiledRooms[gy][gx].exists) {
                drawRoom(&tiledRooms[gy][gx]);
            }
        }
    }
}

/*
 * ------------------------------------------------------------
 * Corridor Carving in the 30x30 map
 * ------------------------------------------------------------
 *
 * The carveCorridor function draws an L-shaped corridor of box-drawing glyphs
 * between two door tiles. 
 * 
 */

/**
 * carveCorridor: Draws a corridor path between (x1,y1) and (x2,y2).
 *  - isHoriz indicates that the overall connection is west→east if true,
 *    or north→south if false (used for picking corner glyphs).
 */
void carveCorridor(int x1, int y1, int x2, int y2, int isHoriz)
{
    // Randomly pick if we move X-first or Y-first to get an L-shape
    int doXFirst = rand() % 2;
    if (x1 == x2) doXFirst = 0; // purely vertical
    if (y1 == y2) doXFirst = 1; // purely horizontal

    // If start == end, just place a single glyph
    if (x1 == x2 && y1 == y2) {
        setCell(x1, y1, isHoriz ? "═" : "║");
        return;
    }

    // (1) Place a "start tile" based on direction
    if (isHoriz) {
        // Overall corridor direction is west→east
        if (doXFirst) {
            setCell(x1, y1, "═"); // horizontal first
        } else {
            // We move vertically first
            if (y2 > y1) setCell(x1, y1, "╗");
            else         setCell(x1, y1, "╝");
        }
    } else {
        // Overall corridor direction is north→south
        if (doXFirst) {
            // We move horizontally first, so corner up or down
            if (x1 > x2) setCell(x1, y1, "╝");
            else         setCell(x1, y1, "╚");
        } else {
            // We move vertically first
            setCell(x1, y1, "║");
        }
    }

    // (2) Phase 1: Move along one axis until we match x2 or y2
    int dx = 0, dy = 0;
    if (doXFirst) {
        dx = (x2 > x1) ? +1 : -1;
        while (x1 != x2) {
            x1 += dx;
            setCell(x1, y1, "═");
        }
    } else {
        dy = (y2 > y1) ? +1 : -1;
        while (y1 != y2) {
            y1 += dy;
            setCell(x1, y1, "║");
        }
    }

    // (3) If there's leftover distance in the other axis => place a pivot corner + Phase 2
    int stillHaveX = (x1 != x2);
    int stillHaveY = (y1 != y2);

    // dx,dy after Phase1 show how we moved in that phase
    int oldDx = dx, oldDy = dy;

    // If we haven't matched both x2,y2 => L-shape
    if (stillHaveX || stillHaveY) {
        // Decide the new direction for Phase2
        int newDx = 0, newDy = 0;
        if (doXFirst && (y1 != y2)) {
            newDy = (y2 > y1) ? +1 : -1;
        } else if (!doXFirst && (x1 != x2)) {
            newDx = (x2 > x1) ? +1 : -1;
        }

        // Place a pivot corner glyph at the current pivot (x1,y1)
        if (newDx != 0 || newDy != 0) {
            // A small mapping from oldDx,oldDy => newDx,newDy to a corner glyph
            if (oldDx > 0 && newDy < 0)      setCell(x1, y1, "╝");
            else if (oldDx > 0 && newDy > 0) setCell(x1, y1, "╗");
            else if (oldDx < 0 && newDy < 0) setCell(x1, y1, "3"); // custom
            else if (oldDx < 0 && newDy > 0) setCell(x1, y1, "╔");
            else if (oldDy > 0 && newDx > 0) setCell(x1, y1, "╚");
            else if (oldDy > 0 && newDx < 0) setCell(x1, y1, "╝");
            else if (oldDy < 0 && newDx > 0) setCell(x1, y1, "╔");
            else if (oldDy < 0 && newDx < 0) setCell(x1, y1, "8"); // custom
            else                             setCell(x1, y1, "?");
        }

        // Phase2
        dx = newDx;
        dy = newDy;
        while (x1 != x2 || y1 != y2) {
            x1 += dx;
            y1 += dy;
            setCell(x1, y1, (dx != 0) ? "═" : "║");
        }
    }

    // (4) Place the "end tile"
    // We see if we ended horizontally or vertically for the last step
    int endDx = dx, endDy = dy;
    setCell(x1, y1, "?"); // put something first, then refine

    if (isHoriz) {
        if (endDy == 0) {
            // purely horizontal end
            setCell(x1, y1, "═");
        } else {
            // corner up or down
            if (endDy > 0) setCell(x1, y1, "╚");
            else           setCell(x1, y1, "╔");
        }
    } else {
        if (endDy == 0) {
            // purely horizontal corner in a vertical corridor
            if (endDx > 0) setCell(x1, y1, "╗");
            else           setCell(x1, y1, "╔");
        } else {
            // purely vertical end
            if (endDy > 0) setCell(x1, y1, "║");
            else           setCell(x1, y1, "K"); // or "║" etc.
        }
    }
}

/*
 * ------------------------------------------------------------
 * Door Placement Between Rooms
 * ------------------------------------------------------------
 */

/**
 * randomWallCoordinate: picks a random coordinate along a wall of the room,
 * skipping the corners (width - 2).
 */
static int randomWallCoordinate(int start, int dimension)
{
    // dimension is either room->width or room->height
    // We skip the corners => [start+1, start+dimension-2]
    return start + 1 + rand() % (dimension - 2);
}

/**
 * placeHorizontalDoors: Handles the case where there's a horizontal corridor 
 * between (gx,gy) and (gx+1,gy).
 */
static void placeHorizontalDoors(int gx, int gy)
{
    TiledRoom *r1 = &tiledRooms[gy][gx];
    TiledRoom *r2 = &tiledRooms[gy][gx + 1];

    // Coordinates of the two rooms in bigMap
    int right1 = r1->x + r1->width - 1;
    int left2  = r2->x;
    
    if (r1->height <= 2 || r2->height <= 2) {
        // Not enough vertical space to place a door skipping corners
        return;
    }
    // Pick random Y positions on each room's vertical wall
    int doorY1 = randomWallCoordinate(r1->y, r1->height);
    int doorY2 = randomWallCoordinate(r2->y, r2->height);

    // Mark each door cell with "╬"
    strcpy(bigMap[doorY1][right1], "╬"); // east wall of R1
    strcpy(bigMap[doorY2][left2],  "╬"); // west wall of R2

    // Carve corridor from the space after R1's wall to the space before R2's wall
    carveCorridor(right1 + 1, doorY1, left2 - 1, doorY2, /*isHoriz=*/1);
}

/**
 * placeVerticalDoors: Handles the case where there's a vertical corridor 
 * between (gx,gy) and (gx,gy+1).
 */
static void placeVerticalDoors(int gx, int gy)
{
    TiledRoom *r1 = &tiledRooms[gy][gx];
    TiledRoom *r2 = &tiledRooms[gy + 1][gx];

    // Coordinates of the two rooms in bigMap
    int bottom1 = r1->y + r1->height - 1;
    int top2    = r2->y;

    if (r1->width <= 2 || r2->width <= 2) {
        // Not enough horizontal space to place a door skipping corners
        return;
    }

    // Pick random X positions on each room's horizontal wall
    int doorX1 = randomWallCoordinate(r1->x, r1->width);
    int doorX2 = randomWallCoordinate(r2->x, r2->width);

    // Mark each door cell with "╬"
    strcpy(bigMap[bottom1][doorX1], "╬"); 
    strcpy(bigMap[top2][doorX2],    "╬");

    // Carve corridor from the space after R1's bottom to the space before R2's top
    carveCorridor(doorX1, bottom1 + 1, doorX2, top2 - 1, /*isHoriz=*/0);
}

/**
 * placeDoorsForCorridors: For each pair of adjacent rooms in the 3x3 grid
 * that share a corridor, place matching door tiles on facing walls,
 * then carve an L-shaped corridor in the bigMap connecting them.
 */
void placeDoorsForCorridors()
{
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (!tiledRooms[gy][gx].exists) 
                continue;

            // If there's a horizontal corridor to (gx+1,gy)
            if (gx < SIZE - 1 && horizontal_corridors[gy][gx] == 1) {
                if (tiledRooms[gy][gx + 1].exists) {
                    placeHorizontalDoors(gx, gy);
                }
            }
            // If there's a vertical corridor to (gx,gy+1)
            if (gy < SIZE - 1 && vertical_corridors[gy][gx] == 1) {
                if (tiledRooms[gy + 1][gx].exists) {
                    placeVerticalDoors(gx, gy);
                }
            }
        }
    }
}

/*
 * ------------------------------------------------------------
 * Player, Treasure, and Exit Placement
 * ------------------------------------------------------------
 */
/**
 * countCorridorsForCell: returns how many corridor connections 
 * this cell (gx, gy) has (both horizontal & vertical).
 */
int countCorridorsForCell(int gx, int gy)
{
    int count = 0;

    // If there's a horizontal corridor from (gx,gy) to (gx+1,gy)
    if (gx < SIZE - 1 && horizontal_corridors[gy][gx])
        count++;
    // If there's a horizontal corridor from (gx-1,gy) to (gx,gy)
    if (gx > 0 && horizontal_corridors[gy][gx - 1])
        count++;

    // If there's a vertical corridor from (gx,gy) to (gx,gy+1)
    if (gy < SIZE - 1 && vertical_corridors[gy][gx])
        count++;
    // If there's a vertical corridor from (gx,gy-1) to (gx,gy)
    if (gy > 0 && vertical_corridors[gy - 1][gx])
        count++;

    return count;
}

/**
 * placePlayerInEdgeRoom: Finds all rooms with exactly 1 corridor connection,
 * picks one at random, and places the player in the center of that TiledRoom 
 * (or anywhere within).
 */
void placePlayerInEdgeRoom()
{
    // Collect all candidate rooms
    int candidates[SIZE*SIZE][2];
    int ccount = 0;

    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (!tiledRooms[gy][gx].exists) continue;

            int corridorCount = countCorridorsForCell(gx, gy);
            if (corridorCount == 1) {
                candidates[ccount][0] = gx;
                candidates[ccount][1] = gy;
                ccount++;
            }
        }
    }

    // If we found none, fallback: place in any existing room
    if (ccount == 0) {
        for (int gy = 0; gy < SIZE && ccount==0; gy++) {
            for (int gx = 0; gx < SIZE && ccount==0; gx++) {
                if (tiledRooms[gy][gx].exists) {
                    candidates[0][0] = gx;
                    candidates[0][1] = gy;
                    ccount = 1;
                }
            }
        }
    }

    // Random pick among the candidates
    int pick = rand() % ccount;
    int gx = candidates[pick][0];
    int gy = candidates[pick][1];

    // Place player near the center of that TiledRoom
    TiledRoom *r = &tiledRooms[gy][gx];

    int px = r->x + r->width / 2;
    int py = r->y + r->height / 2;

    // Make sure it's on top of a "." or some floor
    // For simplicity, we assume interior was "."

    playerX = px;
    playerY = py;

    // Mark the bigMap with "@" for the player
    setCell(playerX, playerY, "@");
}

/**
 * placeTreasureInRandomRoom: picks a random TiledRoom (not the player's),
 * places 'T' in a random interior tile. 
 */
void placeTreasureInRandomRoom()
{
    // Collect all rooms except the one the player is in
    // First, find which room the player is in:
    int playerRoomGX = -1, playerRoomGY = -1;

    // Identify which TiledRoom contains the player's (playerX,playerY)
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (!tiledRooms[gy][gx].exists) continue;
            TiledRoom *r = &tiledRooms[gy][gx];
            if (playerX >= r->x && playerX < r->x + r->width &&
                playerY >= r->y && playerY < r->y + r->height) {
                playerRoomGX = gx;
                playerRoomGY = gy;
                break;
            }
        }
    }

    // Gather all other rooms
    int candidates[SIZE*SIZE][2];
    int ccount = 0;
    for (int gy = 0; gy < SIZE; gy++) {
        for (int gx = 0; gx < SIZE; gx++) {
            if (!tiledRooms[gy][gx].exists) continue;
            if (gx == playerRoomGX && gy == playerRoomGY) continue;
            candidates[ccount][0] = gx;
            candidates[ccount][1] = gy;
            ccount++;
        }
    }

    if (ccount == 0) {
        // Edge case: all rooms are the same or no other rooms
        // Just skip placing treasure
        return;
    }

    // Pick one at random
    int pick = rand() % ccount;
    int gx = candidates[pick][0];
    int gy = candidates[pick][1];
    TiledRoom *r = &tiledRooms[gy][gx];

    // Place T somewhere in that room’s interior
    int tx = r->x + 1 + rand() % (r->width - 2);
    int ty = r->y + 1 + rand() % (r->height - 2);

    setCell(tx, ty, "T");
}

/**
 * findFarthestRoom: BFS from (startGX, startGY) across the 3x3 adjacency,
 * returns the (gx,gy) with the greatest distance that is a room (exists=1).
 */
void findFarthestRoom(int startGX, int startGY, int *outGX, int *outGY)
{
    // Initialize dist
    for (int y=0; y<SIZE; y++) {
        for (int x=0; x<SIZE; x++) {
            dist[y][x] = -1; // unvisited
        }
    }
    dist[startGY][startGX] = 0;

    // BFS queue
    int queue[SIZE*SIZE][2];
    int front = 0, back = 0;

    // Enqueue start
    queue[back][0] = startGX;
    queue[back][1] = startGY;
    back++;

    // BFS
    while (front < back) {
        int gx = queue[front][0];
        int gy = queue[front][1];
        front++;
        int d = dist[gy][gx];

        // Check neighbors
        // Right
        if (gx < SIZE-1 && horizontal_corridors[gy][gx] && rooms[gy][gx+1]) {
            if (dist[gy][gx+1] > d + 1) {
                dist[gy][gx+1] = d + 1;
                queue[back][0] = gx+1;
                queue[back][1] = gy;
                back++;
            }
        }
        // Left
        if (gx > 0 && horizontal_corridors[gy][gx-1] && rooms[gy][gx-1]) {
            if (dist[gy][gx-1] > d + 1) {
                dist[gy][gx-1] = d + 1;
                queue[back][0] = gx-1;
                queue[back][1] = gy;
                back++;
            }
        }
        // Down
        if (gy < SIZE-1 && vertical_corridors[gy][gx] && rooms[gy+1][gx]) {
            if (dist[gy+1][gx] > d + 1) {
                dist[gy+1][gx] = d + 1;
                queue[back][0] = gx;
                queue[back][1] = gy+1;
                back++;
            }
        }
        // Up
        if (gy > 0 && vertical_corridors[gy-1][gx] && rooms[gy-1][gx]) {
            if (dist[gy-1][gx] > d + 1) {
                dist[gy-1][gx] = d + 1;
                queue[back][0] = gx;
                queue[back][1] = gy-1;
                back++;
            }
        }
    }

    // Now find the cell with the largest dist that is a room
    int bestDist = -1;
    int bestGX = startGX, bestGY = startGY;
    for (int gy=0; gy<SIZE; gy++) {
        for (int gx=0; gx<SIZE; gx++) {
            if (rooms[gy][gx] && dist[gy][gx] != -1) {
                if (dist[gy][gx] > bestDist) {
                    bestDist = dist[gy][gx];
                    bestGX = gx;
                    bestGY = gy;
                }
            }
        }
    }

    *outGX = bestGX;
    *outGY = bestGY;
}

void placeExitFarthestFromPlayer()
{
    // Which 3x3 room is the player in?
    int playerRoomGX=-1, playerRoomGY=-1;
    for (int gy=0; gy<SIZE; gy++) {
        for (int gx=0; gx<SIZE; gx++) {
            if (!tiledRooms[gy][gx].exists) continue;
            TiledRoom *r = &tiledRooms[gy][gx];
            if (playerX >= r->x && playerX < r->x + r->width &&
                playerY >= r->y && playerY < r->y + r->height) {
                playerRoomGX = gx;
                playerRoomGY = gy;
                break;
            }
        }
    }

    int farGX, farGY;
    findFarthestRoom(playerRoomGX, playerRoomGY, &farGX, &farGY);
    TiledRoom *farRoom = &tiledRooms[farGY][farGX];

    // Place "E" in a random interior tile
    int ex = farRoom->x + 1 + rand() % (farRoom->width - 2);
    int ey = farRoom->y + 1 + rand() % (farRoom->height - 2);

    setCell(ex, ey, "E");
}

/*
 * ------------------------------------------------------------
 * Printing the 30x30 bigMap
 * ------------------------------------------------------------
 *
 * The printTile function applies special logic to “stretch” certain box-drawing
 * characters horizontally, or to combine them with door glyphs, etc.
 */

/**
 * printTile: Responsible for printing one tile from bigMap[y][x].
 * Optionally, it looks at nextCell to decide if we want to “stretch” the glyph.
 */
void printTile(const char *cell, const char *nextCell)
{
    // Many special cases for wide corridor/door glyphs
    if (strcmp(cell, "═") == 0) {
        printf("══");
    }
    else if (strcmp(cell, "╚") == 0) {
        printf("╚═");
    }
    else if (strcmp(cell, "╔") == 0 && strcmp(nextCell, " ") != 0) { 
        // Possibly a door adjacency
        printf("╔═");
    }
    else if (strcmp(cell, "╬") == 0 && 
            (strcmp(nextCell, "═") == 0 ||
             strcmp(nextCell, "╗") == 0 ||
             strcmp(nextCell, "╝") == 0)) {
        // "╬═" for a door continuing horizontally
        printf("╬═");
    }
    else if (strcmp(cell, "─") == 0) {
        printf("──");
    }
    else if (strcmp(cell, "┌") == 0 && 
            (strcmp(nextCell, "─") == 0 || strcmp(nextCell, "╬") == 0)) {
        printf("┌─");
    }
    else if (strcmp(cell, "└") == 0 && 
            (strcmp(nextCell, "─") == 0 || strcmp(nextCell, "╬") == 0)) {
        printf("└─");
    }
    else if (strcmp(cell, "╬") == 0 &&
            ((strcmp(nextCell, "─") == 0) ||
             (strcmp(nextCell, "┐") == 0) ||
             (strcmp(nextCell, "┘") == 0))) {
        printf("╬─");
    }
    else {
        // Default: print cell + a space
        printf("%s ", cell);
    }
}

/**
 * ncursesPrintTile: Responsible for printing one tile and optionally
 * "stretching" it (like "══") if needed.
 *
 * @param row     The ncurses row
 * @param col     The ncurses column to start printing
 * @param cell    The current tile's string (e.g. "═", "┌", ".")
 * @param nextCell The next tile in the row (to decide if we want to merge horizontally)
 *
 * @return The number of columns printed. Typically 1 or 2.
 */
static int ncursesPrintTile(int row, int col, const char *cell, const char *nextCell)
{
    // Same logic as your printTile() but using mvaddstr.
    // We'll return how many columns we used.

    if (strcmp(cell, "═") == 0) {
        // This "wall" is 2 columns wide
        mvaddstr(row, col, "══");
        return 2;
    }
    else if (strcmp(cell, "╚") == 0) {
        // "╚═" is also 2 columns wide
        mvaddstr(row, col, "╚═");
        return 2;
    }
    else if (strcmp(cell, "╔") == 0) {
        mvaddstr(row, col, "╔═");
        return 2;
    }
    else if (strcmp(cell, "╬") == 0 &&
             (strcmp(nextCell, "═") == 0 ||
              strcmp(nextCell, "╗") == 0 ||
              strcmp(nextCell, "╝") == 0))
    {
        mvaddstr(row, col, "╬═");
        return 2;
    }
    else if (strcmp(cell, "─") == 0) {
        mvaddstr(row, col, "──");
        return 2;
    }
    else if (strcmp(cell, "┌") == 0 &&
             (strcmp(nextCell, "─") == 0 || strcmp(nextCell, "╬") == 0))
    {
        mvaddstr(row, col, "┌─");
        return 2;
    }
    else if (strcmp(cell, "└") == 0 &&
             (strcmp(nextCell, "─") == 0 || strcmp(nextCell, "╬") == 0))
    {
        mvaddstr(row, col, "└─");
        return 2;
    }
    else if (strcmp(cell, "╬") == 0 &&
             (strcmp(nextCell, "─") == 0 ||
              strcmp(nextCell, "┐") == 0 ||
              strcmp(nextCell, "┘") == 0))
    {
        mvaddstr(row, col, "╬─");
        return 2;
    }
    else {
        // Default: print cell as a single character (plus any spacing)
        // e.g. ".", " ", "T", "E", "@", etc.
        mvaddstr(row, col, cell);
        return 1;
    }
}

/**
 * printBigMap: Loops over the 30x30 bigMap and prints each cell using printTile().
 */
void printBigMap()
{
    for (int y = 0; y < BIG_SIZE; y++) {
        for (int x = 0; x < BIG_SIZE; x++) {
            // Look ahead to the next cell in row to decide if we do any wide glyph
            // Guard against x+1 out of bounds by using a conditional.
            const char *nextCell = (x + 1 < BIG_SIZE) ? bigMap[y][x+1] : " ";
            printTile(bigMap[y][x], nextCell);
        }
        printf("\n");
    }
}

/////////////////////////////////////////////
static int isWalkable(const char *cell)
{
    // You might refine this as needed
    if (strcmp(cell, ".") == 0) return 1;
    if (strcmp(cell, " ") == 0) return 1;
    if (strcmp(cell, "T") == 0) return 1;
    if (strcmp(cell, "E") == 0) return 1;
    // corridor glyphs? Doors? It's up to you:
    // if (strcmp(cell, "╬") == 0) return 1; // maybe

    return 0;
}

/**
 * drawBigMapNcurses: Clear screen, then draw the bigMap in ncurses.
 */
void drawBigMapNcurses()
{
    for (int y = 0; y < BIG_SIZE; y++) {

        int cursorX = 0;

        for (int x = 0; x < BIG_SIZE; x++) {
            // Print the entire string stored in bigMap[y][x]
            // mvaddstr(y, x * 2, bigMap[y][x]);

            // Look ahead to the next cell for "stretch" logic
            const char *nextCell = (x + 1 < BIG_SIZE) ? bigMap[y][x+1] : " ";

            // Print the current tile in ncurses
            // This returns how many columns we actually used.
            int usedCols = ncursesPrintTile(y, cursorX, bigMap[y][x], nextCell);

            // Advance cursorX by however many columns we used
            cursorX += 2;
        }
    }
    refresh();
}

/**
 * gameLoopNcurses: 
 *  - Waits for WASD or Q
 *  - Moves player if next cell is walkable
 *  - If we step on 'T', show message, remove 'T'
 *  - If we step on 'E', show message, end game
 */
void gameLoopNcurses()
{
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    // Hide the cursor
    curs_set(0);

    // We placed the player with '@' already. Draw once
    drawBigMapNcurses();

    while (gameRunning) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            // Quit
            gameRunning = 0;
            break;
        }

        int newX = playerX;
        int newY = playerY;

        if (ch == 'w' || ch == 'W') newY--;
        if (ch == 's' || ch == 'S') newY++;
        if (ch == 'a' || ch == 'A') newX--;
        if (ch == 'd' || ch == 'D') newX++;

        // Bounds check
        if (newX < 0 || newX >= BIG_SIZE || newY < 0 || newY >= BIG_SIZE) {
            // Out of range => ignore
            continue;
        }

        // Check if walkable
        if (!isWalkable(bigMap[newY][newX])) {
            // Not walkable => ignore
            continue;
        }

        // It's walkable => move the player
        // Remove old player glyph
        setCell(playerX, playerY, "."); // or whatever floor was
        playerX = newX;
        playerY = newY;

        // If there's treasure (T) or exit (E), handle it
        if (strcmp(bigMap[playerY][playerX], "T") == 0) {
            hasTreasure = 1;
            // Replace the 'T' with '.' floor
            setCell(playerX, playerY, ".");
            // Show a message in a corner
            mvprintw(BIG_SIZE+1, 0, "You got the treasure!");
        }
        else if (strcmp(bigMap[playerY][playerX], "E") == 0) {
            if (!hasTreasure) {
                mvprintw(BIG_SIZE+2, 0, "You found the exit... but you have no treasure!");
            } else {
                // We have the treasure, so we truly escaped
                mvprintw(BIG_SIZE+2, 0, "You escaped the dungeon!");
                gameRunning = 0;
            }
        }

        // Place the new player glyph
        setCell(playerX, playerY, "@");

        // Redraw
        drawBigMapNcurses();
    }

    // End curses mode
    endwin();
}

/*
 * ------------------------------------------------------------
 * main: Demonstration
 * ------------------------------------------------------------
 */
int main(void)
{
    // 1) Generate the 3x3 "macro" dungeon layout
    generateMaze();
    // Optionally: printMaze();

    // 2) Prepare and build the 30x30 "tiled" map
    clearBigMap();
    positionRoomsInQuadrants();
    drawAllRooms();
    placeDoorsForCorridors();

    // 3) Place player, treasure, exit
    placePlayerInEdgeRoom();
    placeTreasureInRandomRoom();
    placeExitFarthestFromPlayer();

    // 4) Start ncurses main loop
    gameLoopNcurses();

    // If you want a final message outside curses:
    if (!gameRunning) {
        if (hasTreasure) {
            printf("You escaped with the treasure!\n");
        } else {
            printf("You quit or left without treasure.\n");
        }
    }
    return 0;
}
