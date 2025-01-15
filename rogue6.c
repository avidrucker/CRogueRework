#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define SIZE 3
#define BIG_SIZE 30
#define SUBGRID_SIZE 10
#define MIN_ROOM_DIM 5
#define MAX_ROOM_DIM 9

int rooms[SIZE][SIZE] = {0};
int horizontal_corridors[SIZE][SIZE] = {0};
int vertical_corridors[SIZE][SIZE] = {0};

char bigMap[BIG_SIZE][BIG_SIZE][4];

typedef struct
{
  int x, y;
  int width, height;
  int exists;
} TiledRoom;

TiledRoom tiledRooms[SIZE][SIZE];

void shuffle(int *array, size_t n)
{
  for (size_t i = 0; i < n - 1; i++)
  {
    size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
    int t = array[j];
    array[j] = array[i];
    array[i] = t;
  }
}

void recursiveBacktracking(int x, int y, int *room_count, int max_rooms)
{
  rooms[y][x] = 1;
  (*room_count)++;

  int directions[] = {0, 1, 2, 3}; // up,right,down,left
  shuffle(directions, 4);

  for (int i = 0; i < 4; i++)
  {
    int nx = x, ny = y;
    switch (directions[i])
    {
    case 0:
      ny--;
      break; // up
    case 1:
      nx++;
      break; // right
    case 2:
      ny++;
      break; // down
    case 3:
      nx--;
      break; // left
    }
    if (nx < 0 || nx >= SIZE || ny < 0 || ny >= SIZE)
      continue;
    if (rooms[ny][nx])
      continue; // Already a room

    // Mark corridor always from smaller cell
    if (ny > y)
    { // going down
      vertical_corridors[y][x] = 1;
    }
    else if (ny < y)
    { // going up
      vertical_corridors[ny][x] = 1;
    }
    else if (nx > x)
    { // going right
      horizontal_corridors[y][x] = 1;
    }
    else if (nx < x)
    { // going left
      horizontal_corridors[y][nx] = 1;
    }

    recursiveBacktracking(nx, ny, room_count, max_rooms);
    if (*room_count >= max_rooms)
      return;
  }
}

void generateMaze()
{
  srand(time(NULL));
  int room_count = 0;
  int max_rooms = 6 + rand() % 4; // Random number between 6 and 9
  int startX = rand() % SIZE;     // Random starting position
  int startY = rand() % SIZE;

  recursiveBacktracking(startX, startY, &room_count, max_rooms);
}

void printMaze()
{
  // Print the larger grid
  for (int y = 0; y < SIZE; y++)
  {
    // Print the row with rooms and horizontal corridors
    for (int x = 0; x < SIZE; x++)
    {
      if (rooms[y][x])
      {
        printf("R");
      }
      else
      {
        printf("#");
      }

      if (x < SIZE - 1)
      {
        if (horizontal_corridors[y][x] && rooms[y][x] && rooms[y][x + 1])
        {
          printf("---");
        }
        else
        {
          printf("###");
        }
      }
    }
    printf("\n");

    // Print the row with vertical corridors
    if (y < SIZE - 1)
    {
      for (int x = 0; x < SIZE; x++)
      {
        if (vertical_corridors[y][x] && rooms[y][x] && rooms[y + 1][x])
        {
          printf("|   ");
        }
        else
        {
          printf("#   ");
        }
      }
      printf("\n");
    }
  }
}

void clearBigMap()
{
  for (int y = 0; y < BIG_SIZE; y++)
  {
    for (int x = 0; x < BIG_SIZE; x++)
    {
      strcpy(bigMap[y][x], " ");
    }
  }
}

void positionRoomsInQuadrants()
{
  int margin = 1; // Keep 1-tile border around every quadrant

  for (int gy = 0; gy < SIZE; gy++)
  {
    for (int gx = 0; gx < SIZE; gx++)
    {
      tiledRooms[gy][gx].exists = 0;
      if (!rooms[gy][gx])
        continue; // skip if not a room

      tiledRooms[gy][gx].exists = 1;

      int w = MIN_ROOM_DIM + rand() % (MAX_ROOM_DIM - MIN_ROOM_DIM + 1);
      int h = MIN_ROOM_DIM + rand() % (MAX_ROOM_DIM - MIN_ROOM_DIM + 1);

      int quadX = gx * SUBGRID_SIZE;
      int quadY = gy * SUBGRID_SIZE;

      // Subgrid is 10×10. We want 1 tiles of padding on left/right and top/bottom
      // => effectively we only have (10 - 2) = 8 tiles for the interior max left
      int availableW = SUBGRID_SIZE - w - 2 * margin;
      int availableH = SUBGRID_SIZE - h - 2 * margin;

      if (availableW < 0)
        availableW = 0; // safeguard
      if (availableH < 0)
        availableH = 0;

      int roomLeft = quadX + margin + rand() % (availableW + 1);
      int roomTop = quadY + margin + rand() % (availableH + 1);

      tiledRooms[gy][gx].x = roomLeft;
      tiledRooms[gy][gx].y = roomTop;
      tiledRooms[gy][gx].width = w;
      tiledRooms[gy][gx].height = h;
    }
  }
}

void drawRoom(TiledRoom *r)
{
  int left = r->x;
  int top = r->y;
  int right = left + r->width - 1;
  int bottom = top + r->height - 1;

  // Corners
  strcpy(bigMap[top][left], "┌");
  strcpy(bigMap[top][right], "┐");
  strcpy(bigMap[bottom][left], "└");
  strcpy(bigMap[bottom][right], "┘");

  // Top/bottom edges
  for (int x = left + 1; x < right; x++)
  {
    strcpy(bigMap[top][x], "─");
    strcpy(bigMap[bottom][x], "─");
  }

  // Left/right edges
  for (int y = top + 1; y < bottom; y++)
  {
    strcpy(bigMap[y][left], "│");
    strcpy(bigMap[y][right], "│");
  }

  // Fill interior with "." as before
  for (int y = top + 1; y < bottom; y++)
  {
    for (int x = left + 1; x < right; x++)
    {
      strcpy(bigMap[y][x], ".");
    }
  }
}

void drawAllRooms()
{
  for (int gy = 0; gy < SIZE; gy++)
  {
    for (int gx = 0; gx < SIZE; gx++)
    {
      if (tiledRooms[gy][gx].exists)
      {
        drawRoom(&tiledRooms[gy][gx]);
      }
    }
  }
}

// A helper to set a cell’s debug string quickly:
static void setCell(int x, int y, const char *s)
{
  // Safely copy up to 3 chars plus null terminator
  strncpy(bigMap[y][x], s, 3);
  bigMap[y][x][3] = '\0';
}

// isHoriz indicates that the corridor is joining two rooms from west to east
// if false, it is joining two rooms from north to south
void carveCorridor(int x1, int y1, int x2, int y2, int isHoriz)
{
  // Randomly pick doXFirst if corridor is truly L-shaped.
  int doXFirst = rand() % 2;
  if (x1 == x2)
    doXFirst = 0; // purely vertical
  if (y1 == y2)
    doXFirst = 1; // purely horizontal

  if (x1 == x2 && y1 == y2)
  {
    if (isHoriz)
    {
      // place down a horizontal corridor and we're done
      setCell(x1, y1, "═");
    }
    else
    {
      // place down a vertical corridor and we're done
      setCell(x1, y1, "║");
    }
    return;
  }

  // --------------------------------------------------------------------
  // 1) Place a "start tile" debug letter
  // --------------------------------------------------------------------
  if (isHoriz)
  {
    // Corridor came from west→east overall
    if (doXFirst)
    {
      // We'll move horizontally first
      setCell(x1, y1, "═");
    }
    else
    {
      if (y2 > y1)
        setCell(x1, y1, "╗");
      else
        setCell(x1, y1, "╝");
    }
  }
  else
  {
    // Corridor came from north→south
    if (doXFirst)
    {
      if (x1 > x2)
        setCell(x1, y1, "╝");
      else
        setCell(x1, y1, "╚");
    }
    else
    {
      // We'll move vertically first
      setCell(x1, y1, "║");
    }
  }

  // For tracking the direction used in Phase1
  // (A) PHASE1 direction
  int dx = 0, dy = 0;
  if (doXFirst)
  {
    dx = (x2 > x1) ? +1 : -1; // horizontal
  }
  else
  {
    dy = (y2 > y1) ? +1 : -1; // vertical
  }

  // --------------------------------------------------------------------
  // 2) PHASE 1
  // --------------------------------------------------------------------
  if (doXFirst)
  {
    // Move horizontally from (x1,y1) to (x2,y1)
    dx = (x2 > x1) ? 1 : -1;
    dy = 0;
    while (x1 != x2)
    {
      x1 += dx;
      setCell(x1, y1, "═"); // horizontal
    }
  }
  else
  {
    // Move vertically from (x1,y1) to (x1,y2)
    dx = 0;
    dy = (y2 > y1) ? 1 : -1;
    while (y1 != y2)
    {
      y1 += dy;
      setCell(x1, y1, "║"); // vertical
    }
  }

  // Now (x1,y1) is at the "pivot" if there's still leftover distance in the other axis
  // But we might already be done if it's a straight corridor.

  // --------------------------------------------------------------------
  // 3) If there's leftover distance, place a pivot corner debug letter
  //    Then do PHASE 2 in the other axis
  // --------------------------------------------------------------------
  // Because we updated (x1,y1) all the way to (x2,y2), let's see if we’re done:
  // If after Phase1, x1==x2 AND y1==y2, it's a straight corridor, no pivot.
  // If not, we likely had an L shape, so we place a corner & keep going.
  // But in your code, you might do it differently. Let’s be explicit:

  // Actually, if we forced doXFirst, we used up all horizontal difference,
  // so if y1 != y2, that means we STILL have vertical difference to do => L shape.
  // Similarly, if we doYFirst, we used up all vertical difference, so if x1!=x2 => L shape.
  // We'll still handle the second leg plus the pivot letter.

  // But to highlight the difference, we can do:
  int stillHaveX = (x1 != x2);
  int stillHaveY = (y1 != y2);
  if (stillHaveX || stillHaveY)
  {
    // We do a 2nd phase. But we want a corner letter at (x1,y1) first.

    // (B) Figure out leftover axis => PHASE2 direction
    int newDx = 0, newDy = 0;
    if (doXFirst && (y1 != y2))
    {
      // we still have vertical left
      newDy = (y2 > y1) ? +1 : -1;
    }
    else if (!doXFirst && (x1 != x2))
    {
      // still have horizontal
      newDx = (x2 > x1) ? +1 : -1;
    }

    // Choose a pivot corner letter. For example:
    // A => horizontal→up, B => horizontal→down, C => vertical→left, D => vertical→right
    // You can refine these to match your corner definitions, or do more granular checks.
    if (newDx != 0 || newDy != 0)
    {
      if (dx > 0 && newDy < 0)
        setCell(x1, y1, "╝"); // right→up
      else if (dx > 0 && newDy > 0)
        setCell(x1, y1, "╗"); // upper-right
      else if (dx < 0 && newDy < 0)
        setCell(x1, y1, "3");
      else if (dx < 0 && newDy > 0)
        setCell(x1, y1, "╔"); // upper-left
      else if (dy > 0 && newDx > 0)
        setCell(x1, y1, "╚"); // bottom-left
      else if (dy > 0 && newDx < 0)
        setCell(x1, y1, "╝"); // bottom-right
      else if (dy < 0 && newDx > 0)
        setCell(x1, y1, "╔"); // upper-left
      else if (dy < 0 && newDx < 0)
        setCell(x1, y1, "8");
      else
        setCell(x1, y1, "?");
    }

    // Now do Phase 2
    if (doXFirst)
    {
      // we do vertical now
      dy = newDy;
      dx = 0;
      while (y1 != y2)
      {
        y1 += dy;
        setCell(x1, y1, "║");
      }
    }
    else
    {
      // we do horizontal now
      dx = newDx;
      dy = 0;
      while (x1 != x2)
      {
        x1 += dx;
        setCell(x1, y1, "═");
      }
    }
  }

  // --------------------------------------------------------------------
  // 4) Place the "end tile" debug letter
  //    Possibly a corner if we approach from N, S, E, W and isHoriz says otherwise
  // --------------------------------------------------------------------
  // We'll see how we arrived at (x2,y2). That means the last movement was dx,dy from the line above.
  // If we came from “north” => dy=1, from “south” => dy=-1, from “east” => dx=-1, from “west” => dx=1
  // Then combine with `isHoriz` to pick a letter that denotes a corner or an end piece.
  // For brevity, we’ll do a small if-else:

  // After Phase 2, we have final (x1,y1),
  // and the final step used (dx,dy).
  // We want to see if we are "purely vertical" or "purely horizontal"
  // for that last step, or if it is a corner scenario.

  int endDx = dx;
  int endDy = dy;

  setCell(x1, y1, "?");

  if (isHoriz)
  {
    if (endDy == 0)
    {
      if (endDx > 0)
        setCell(x1, y1, "═");
      else if (endDx < 0)
        setCell(x1, y1, "═");
      else
        setCell(x1, y1, "C");
    }
    else
    {
      if (endDy > 0)
        setCell(x1, y1, "╚");
      else if (endDy < 0)
        setCell(x1, y1, "╔");
      else
        setCell(x1, y1, "F");
    }
  }
  else
  {
    if (endDy == 0)
    {
      if (endDx > 0)
        setCell(x1, y1, "╗");
      else if (endDx < 0)
        setCell(x1, y1, "╔");
      else
        setCell(x1, y1, "I");
    }
    else
    {
      if (endDy > 0)
        setCell(x1, y1, "║");
      else if (endDy < 0)
        setCell(x1, y1, "K");
      else
        setCell(x1, y1, "L");
    }
  }
}

// Place "#" doors on facing walls where corridors exist
void placeDoorsForCorridors()
{
  for (int gy = 0; gy < SIZE; gy++)
  {
    for (int gx = 0; gx < SIZE; gx++)
    {
      if (!tiledRooms[gy][gx].exists)
        continue;

      TiledRoom *r1 = &tiledRooms[gy][gx];

      // Horizontal corridor to (gx+1, gy)
      if (gx < SIZE - 1 && horizontal_corridors[gy][gx] == 1)
      {
        TiledRoom *r2 = &tiledRooms[gy][gx + 1];
        if (!r2->exists)
          continue;

        int left1 = r1->x;
        int top1 = r1->y;
        int right1 = left1 + r1->width - 1;
        int bottom1 = top1 + r1->height - 1;

        int left2 = r2->x;
        int top2 = r2->y;
        int right2 = left2 + r2->width - 1;
        int bottom2 = top2 + r2->height - 1;

        // x-coords are the walls
        int doorX1 = right1;
        int doorX2 = left2;

        if (r1->height > 2 && r2->height > 2)
        {
          int doorY1 = top1 + 1 + rand() % (r1->height - 2);
          int doorY2 = top2 + 1 + rand() % (r2->height - 2);

          // place the door tiles
          strcpy(bigMap[doorY1][doorX1], "╬"); // east wall of R1
          strcpy(bigMap[doorY2][doorX2], "╬"); // west wall of R2

          // Now carve corridor from (doorX1, doorY1) to (doorX2, doorY2)
          carveCorridor(doorX1 + 1, doorY1, doorX2 - 1, doorY2, 1);
        }
      }

      // Vertical corridor to (gx, gy+1)
      if (gy < SIZE - 1 && vertical_corridors[gy][gx] == 1)
      {
        TiledRoom *r2 = &tiledRooms[gy + 1][gx];
        if (!r2->exists)
          continue;

        int left1 = r1->x;
        int top1 = r1->y;
        int right1 = left1 + r1->width - 1;
        int bottom1 = top1 + r1->height - 1;

        int left2 = r2->x;
        int top2 = r2->y;
        int right2 = left2 + r2->width - 1;
        int bottom2 = top2 + r2->height - 1;

        // x-coords are the walls
        int doorY1 = bottom1;
        int doorY2 = top2;

        if (r1->width > 2 && r2->width > 2)
        {
          int doorX1 = left1 + 1 + rand() % (r1->width - 2);
          int doorX2 = left2 + 1 + rand() % (r2->width - 2);

          strcpy(bigMap[doorY1][doorX1], "╬"); // bottom wall of r1
          strcpy(bigMap[doorY2][doorX2], "╬"); // top wall of r2

          // Now carve corridor from (doorX1, doorY1) to (doorX2, doorY2)
          carveCorridor(doorX1, doorY1 + 1, doorX2, doorY2 - 1, 0);
        }
      }
    }
  }
}

void printTile(const char *cell, const char *nextCell)
{
  if (strcmp(cell, "═") == 0)
    printf("══");
  else if (strcmp(cell, "╚") == 0)
    printf("╚═");
  else if (strcmp(cell, "╔") == 0 && strcmp(nextCell, " ") != 0) // door
    printf("╔═");
  else if (strcmp(cell, "╬") == 0 && (strcmp(nextCell, "═") == 0 ||
                                              strcmp(nextCell, "╗") == 0 ||
                                              strcmp(nextCell, "╝") == 0)) //// door
    printf("╬═");
  else if (strcmp(cell, "─") == 0)
    printf("──");
  else if (strcmp(cell, "┌") == 0 && (strcmp(nextCell, "─") == 0 ||
                                              strcmp(nextCell, "╬") == 0))
    printf("┌─");
  else if (strcmp(cell, "└") == 0 && (strcmp(nextCell, "─") == 0 ||
                                              strcmp(nextCell, "╬") == 0))
    printf("└─");
  else if (strcmp(cell, "╬") == 0 && (strcmp(nextCell, "─") == 0 ||
                                              strcmp(nextCell, "┐") == 0) ||
           strcmp(nextCell, "┘") == 0)
    printf("╬─");
  else
    printf("%s ", cell);
}

void printBigMap()
{
  for (int y = 0; y < BIG_SIZE; y++)
  {
    for (int x = 0; x < BIG_SIZE; x++)
    {
      // print a character + a space
      printTile(bigMap[y][x], bigMap[y][x + 1]);
    }
    printf("\n");
  }
}

int main()
{
  srand(time(NULL));
  generateMaze(); // Creates 3x3 map with up to 6-9 rooms
  // printMaze();     // (Optional) See the "R---R" debug output

  // Now generate the 90x90 tiled map:
  clearBigMap();
  positionRoomsInQuadrants();
  drawAllRooms();
  placeDoorsForCorridors();
  printBigMap();

  return 0;
}
