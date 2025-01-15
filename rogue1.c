#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 3

int grid[SIZE][SIZE] = {0};
int rooms[SIZE][SIZE] = {0};
int corridors[SIZE][SIZE] = {0};

void shuffle(int *array, size_t n) {
    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }
}

void recursiveBacktracking(int x, int y, int *room_count) {
    grid[y][x] = 1;
    rooms[y][x] = 1;
    (*room_count)++;

    int directions[] = {0, 1, 2, 3};
    shuffle(directions, 4);
    
    for (int i = 0; i < 4; i++) {
        int nx = x, ny = y;
        switch (directions[i]) {
            case 0: ny -= 1; break; // Up
            case 1: nx += 1; break; // Right
            case 2: ny += 1; break; // Down
            case 3: nx -= 1; break; // Left
        }
        
        if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && grid[ny][nx] == 0) {
            corridors[(y + ny) / 2][(x + nx) / 2] = 1;
            recursiveBacktracking(nx, ny, room_count);
        }
    }
}

void generateMaze() {
    srand(time(NULL));
    int room_count = 0;
    int startX = rand() % SIZE;
    int startY = rand() % SIZE;
    
    recursiveBacktracking(startX, startY, &room_count);

    // Randomly reduce the number of rooms to be between 6 and 9
    int total_rooms = 6 + rand() % 4;
    while (room_count > total_rooms) {
        int x = rand() % SIZE;
        int y = rand() % SIZE;
        if (rooms[y][x] == 1) {
            rooms[y][x] = 0;
            room_count--;
        }
    }
}

void printMaze() {
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            if (rooms[y][x]) {
                printf("R ");
            } else if (corridors[y][x]) {
                printf("C ");
            } else {
                printf("# ");
            }
        }
        printf("\n");
    }
}

int main() {
    generateMaze();
    printMaze();
    return 0;
}

