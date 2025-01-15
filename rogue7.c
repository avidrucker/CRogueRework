#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

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

// These store which 3x3 cells actually have rooms (1) and which corridors connect them
int rooms[SIZE][SIZE] = {0};
int horizontal_corridors[SIZE][SIZE] = {0};
int vertical_corridors[SIZE][SIZE]   = {0};

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

/*
 * ------------------------------------------------------------
 * main: Demonstration
 * ------------------------------------------------------------
 */
int main(void)
{
    // 1) Generate the 3x3 "macro" dungeon layout
    generateMaze();  
    // printMaze();  // (Optional) debug output of 3x3 layout

    // 2) Prepare and build the 30x30 "tiled" map
    clearBigMap();
    positionRoomsInQuadrants();
    drawAllRooms();
    placeDoorsForCorridors();

    // 3) Print the final tiled dungeon
    printBigMap();

    return 0;
}
