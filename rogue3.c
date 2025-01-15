#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 3

int grid[SIZE][SIZE] = {0};
int rooms[SIZE][SIZE] = {0};
int horizontal_corridors[SIZE][SIZE] = {0}; // For "---" connections
int vertical_corridors[SIZE][SIZE] = {0};   // For "|" connections

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
            // Only add corridors if the destination cell will become a room
            if (directions[i] == 0 || directions[i] == 2) {
                vertical_corridors[(y + ny) / 2][x] = 1; // Add vertical corridor
            } else if (directions[i] == 1 || directions[i] == 3) {
                horizontal_corridors[y][(x + nx) / 2] = 1; // Add horizontal corridor
            }

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

int main() {
    generateMaze();
    printMaze();
    return 0;
}

