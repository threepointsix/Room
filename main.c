#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <ftw.h>
#include <limits.h>
#include <stdbool.h>

#define MAXFD 100
#define MAX_LINE 100
#define MAX_ITEMS 2
#define MAX_PATH 256
#define MAX_PATH_LENGTH 1000
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

int global_dir_number = 0; // I used nftw() to create a map from directory structure, so one global variable is necessary

// Structures

typedef struct item {
  int item_number; // Item's unique ID
  int allocated_to_room; // Room ID, to which this item is assigned
} item;

typedef struct player { // Player can only carry 2 items
  int current_items[MAX_ITEMS]; // Current items, that player is carrying
  int currentRoom;
} player;


typedef struct node {  // Room is a vertice (node) in the graph.
  int vertex; // Unique room ID
  int allocated_items[MAX_ITEMS]; // IDs of items, that are assigned to that room
  int current_items[MAX_ITEMS]; // IDs of current items
  struct node *next; // array of rooms, that are adjacent to that room
} room;


typedef struct Graph { // Graph is a map, where the game is played.
  int numVertices; // Total number of rooms (vertices)
  int numItems; // Total number of items on the map
  int numCorrectItems; // Total number of items in the correct place. Game ends, when [numItems == numCorrectItems]
  player play; // It is single player game, so only 1 player on the map :)
  item *items; // Array of all items on the map (needed to save the map into the file)
  struct node** adjLists; // Simple adjacency list of the graph
  char *backupPath;

  pthread_t threads[2]; // Game thread and autosave thread (here also should be not implemented thread, that waits for SIGUSR1 signal)
} graph;

typedef struct path {
  pthread_t tid;
  unsigned int seed;
  int from_room;
  int to_room;
  int path_length;
  int path_rooms[MAX_PATH_LENGTH];
  graph *map;
  pthread_mutex_t *mx_map;
} path;

// ------ Declaring all functions

void usage();

// -- Graph (map) functions

struct node* createNode(int); // Adding a node (room) to the graph (map)
struct Graph* createAGraph(int vertices); // Creating a graph (map)
void addEdge(struct Graph* graph, int s, int d); // Adding an edge between nodes (door between rooms)
void printGraph(struct Graph* graph); // Additional (unused) function to print whole graph and items info
int isThereEdge(graph *gr, int v1, int v2); // Returns 0 if there is no door between rooms; Returns 1 if there is
int numberAdjVertices(graph *gr, int n); // Number of doors in the room

// -- Main menu functions

int mainMenu(char *backupPath);  // All functions are collected there
void mainMenuInstructions(); // Printing a list of commands available in the main menu
void readMap(char *path, char *backupPath); // Reading a map from path, and starting a new game (i.e. random items in random spots && player initally carries no items)
void randMap(int n, char *path); // Creating random map with [n] rooms
void startNewGame(graph *map, char *backupPath); // Starts a new game
void startLoadGame(char *path, char *backupPath); // Loading the game (i.e. items positions are taken from the file && player current items && items assignment to rooms)
void mapFromDir(char *pathd, char *pathf); // Creating a map, which copies directory structure of [pathd] file, and saves the created map into [pathf] file

void recursiveCreateMap(graph *map, char *path, int roomNumber, int *currentRooms); // Used in [mapFromDir]
int currentDirectorySubfolders(); // Calculates number of subfolders in current (!) directory
void shuffle(int *array, size_t n); // Shuffles the given array of size [n]
int walk(const char *name, const struct stat *s, int type, struct FTW *f); // Returns the total number of rooms needed to create a map in [mapFromDir] function

// -- In-game functions

void *inGame(void *arg); // All functions are collected there
void gameInstructions(); // Printing a list of commands available in-game
void currentGameOutput(graph *gr); // Printing current game state 
void moveTo(graph *map, int to); // Move to room 
void pickUp(graph *map, int item_number); // Pick up item
void dropItem(graph *map, int item_number); // Drop item
void gameEnd(graph *gr); // It checks if total number of items on the map is equal to the total number of items in correct spots

void kThreadMethod(graph *gr, int threadAmount, int from_room, int to_room); // Seeks for shortest path using k threads
void *findPath(void *voidPtr); // Worker function

void *autosave(void *arg); // Partly working (autosaving each 60 seconds, doesn't restart on manual save)
void *waitingForSIGUSR(void *arg); // Not implemented

// -- Reading commands functions

char* singlePathFromLine(char *line, char *command); // Reading a single path from command line
char* pathDFromLine(char *line, char *command); // Reading the first path
char* pathFFromLine(char *line, char *path_d, char *command); // Reading the second path
int singleIntFromLine(char *line, char *command); // Reading single integer

// -- Saving and loading game (marshalling)

void saveGame(graph *map, char *path); // Saving game by manually writing all data to the binary file
graph *loadGame(char *path); // Load game by reading all data in the right order (as it was written) from the binary file


int main(int argc, char** argv) {    

    if (argc > 3) {
      usage();
      return EXIT_FAILURE;
    }

    char *backupEnv = getenv("GAME_AUTOSAVE");
    srand(time(NULL));
    
    if (argc > 1) {
      int c;
      while ((c = getopt(argc, argv, "b:")) != -1) {
        switch (c) {
          case 'b':
            printf("\nEvery 60 seconds your game backup will be saved to: [%s]\n", optarg);
            mainMenu(optarg);
            break;
          default:
            usage();
            return EXIT_FAILURE;
        }
      }
    }
    else if (backupEnv != NULL) {
      printf("\nEvery 60 seconds your game backup will be saved to: [%s]\n", backupEnv);
      mainMenu(backupEnv);
    }
    else {
      char *homePath = (char*)malloc(strlen(getenv("HOME")) + strlen("/game-autosave") + 2);
      homePath = strcat(getenv("HOME"), "/game-autosave");
      printf("\nEvery 60 seconds your game backup will be saved to: [%s]\n", homePath);
      mainMenu(homePath);
    }

    return EXIT_SUCCESS;
}

// -------------------------- Output functions (not interesting) ------------------------

void usage() {
  printf("\nUsage: ./main -b [path]\n");
}

void mainMenuInstructions() {
  printf("\n---------------Main menu---------------\n\n");
  printf("read-map [path]\n");
  printf("    Read a map from a file given as an argument and starts a new game on it.\n\n");
  printf("map-from-dir-tree [path-d] [path-f]\n");
  printf("    Generates a map that mirrors a directory structure of path-d and its subdirectories.\n\n");
  printf("generate-random-map [n] [path]\n");
  printf("    Generates a random map of n rooms and writes the result in path.\n\n");
  printf("load-game [path]\n");
  printf("    Loads the game state from path.\n\n");
  printf("exit\n");
  printf("    Exit the game.\n\n");
}

void gameInstructions() {
  printf("\n---------------Game---------------\n\n");
  printf("move-to [x]\n\n");
  printf("pick-up [y]\n\n");
  printf("drop [z]\n\n");
  printf("save [path]\n");
  printf("    Save a current game to the file\n\n");
  printf("find-path [k] [x]\n");
  printf("    Find a shortest path to x using k threads\n\n");
  printf("quit\n\n");
}

void currentGameOutput(graph *map) {
  printf("\nTotal number of items: %d \nTotal number of items in correct spots: %d\n", map->numItems, map->numCorrectItems);

  printf("You are currently in the room: %d\n", map->play.currentRoom);
  printf("You can travel to the rooms:");
  room *temp = map->adjLists[map->play.currentRoom];
  while (temp) {
    printf("%d ", temp->vertex);
    temp = temp->next;
  }
  printf("\n");
  printf("Your current items:");
  for (int k = 0; k < MAX_ITEMS; k++) {
    if (map->play.current_items[k] != -1) printf("%d ", map->play.current_items[k]);
  }
      printf("\n");

      printf("Items in this room:");
      for (int k = 0; k < MAX_ITEMS; k++) {
        if (map->adjLists[map->play.currentRoom]->current_items[k] != -1) printf("%d ", map->adjLists[map->play.currentRoom]->current_items[k]);
      }
      printf("\n");

      printf("Items assigned to this room:");
      for (int k = 0; k < MAX_ITEMS; k++) {
        if (map->adjLists[map->play.currentRoom]->allocated_items[k] != -1) printf("%d ", map->adjLists[map->play.currentRoom]->allocated_items[k]);
      }
      printf("\n");
}

// ------------------------ Functions for taking paths and integers from given commands -----------------------

char* singlePathFromLine(char *line, char *command) {
  char *path = malloc(sizeof(char)*(strlen(line) - strlen(command)));
  int j = 0;
  for (int i = strlen(command) + 1; i < strlen(line) - 1; i++, j++) { // Storing path as a string
    path[j] = line[i];
  }
  path[j] = '\0'; 
  return path;
}

char* pathDFromLine(char *line, char *command) {
  char *path_d  = malloc(sizeof(char)*(strlen(line) - strlen(command)));
  int j = 0;
  for (int i = strlen(command) + 1; i < strlen(line); i++, j++) {
    if (line[i] == ' ') break;
    path_d[j] = line[i];
    }
  path_d[j] = '\0';
  return path_d;
}

char *pathFFromLine(char *line, char *path_d, char *command) {
  char *path_f = malloc(sizeof(char)*(strlen(line) - strlen(command) - strlen(path_d)));
  int j = 0;
  for (int i = strlen(command) + strlen(path_d) + 2; i < strlen(line) - 1; i++, j++) {
    path_f[j] = line[i];
  }
  path_f[j] = '\0';
  return path_f;
}

int singleIntFromLine(char *line, char *command) {
  char *num_string = malloc(sizeof(char)*strlen(line)-strlen(command));
  int num;
  strcpy(num_string, pathDFromLine(line, command));
  num = atoi(num_string);
  return num;
}

// ------------------------ Saving and loading game (marshalling) --------------------------


void saveGame(graph *map, char *path) {
  FILE *map_file;
  map_file = fopen(path, "wb"); // Creating the file [path]

  if (map_file == NULL) {
     fprintf(stderr, "Error opening file\n");
     return;
  }

  fwrite(&map->numVertices, sizeof(int), 1, map_file); // 1. Total number of rooms
  fwrite(&map->numItems, sizeof(int), 1, map_file); // 2. Total number of items
  fwrite(&map->numCorrectItems, sizeof(int), 1, map_file); // 3. Total number of items in correct place
  fwrite(&map->play.currentRoom, sizeof(int), 1, map_file); // 4. Current room ID
  fwrite(&map->play, sizeof(player), 1, map_file); // 5. Player's current items

  for (int i = 0; i < map->numItems; i++) {
    fwrite(&map->items[i], sizeof(map->items[i]), 1, map_file); // Info about all items (IDs and assigned rooms)
  }

  for (int i = 0; i < map->numVertices; i++) {
    size_t length = numberAdjVertices(map, i);
    fwrite(&length, sizeof(length), 1, map_file); // Number of doors in room[i]

    room *temp = map->adjLists[i];

    for(int j = 0; j < length; j++) {
      fwrite(&temp->vertex, sizeof(temp->vertex), 1, map_file); // Adjacent rooms of room[i]
      temp = temp->next;
    } 
  }

  for (int i = 0; i < map->numVertices; i++) {
    room *temp = map->adjLists[i];
    while(temp) {
      fwrite(&temp->current_items, sizeof(temp->current_items), 1, map_file); // Current rooms in room[i]
      temp = temp->next;
    }
  }


  printf("\nSaved the current game to the [%s]\n", path);
  fclose(map_file);
}

graph *loadGame(char *path) { // Loading is performed in the same order, as data was written in [saveGame] function
  FILE *map_file;
  map_file = fopen(path, "rb");

  if (map_file == NULL) {
    fprintf(stderr, "Error opening file\n");
  }

  int map_size = 0;

  fread(&map_size, sizeof(int), 1, map_file);

  graph *map = createAGraph(map_size);

  fread(&map->numItems, sizeof(int), 1, map_file);
  fread(&map->numCorrectItems, sizeof(int), 1, map_file);
  fread(&map->play.currentRoom, sizeof(int), 1, map_file);
  fread(&map->play, sizeof(player), 1, map_file);

  map->items = malloc(sizeof(item) * map->numItems);

  for (int i = 0; i < map->numItems; i++) {
    fread(&map->items[i], sizeof(map->items[i]), 1, map_file); 
  }

  for (int i = 0; i < map->numVertices; i++) {
    size_t length;
    fread(&length, sizeof(length), 1, map_file);
    for (int j = 0; j < length; j++) {
      int ver;
      fread(&ver, sizeof(map->adjLists[j]->vertex), 1, map_file);
      if (isThereEdge(map, i, ver) == 0) addEdge(map, i, ver);
    }    
  }

  for (int i = 0; i < map->numVertices; i++) {
    room *temp = map->adjLists[i];
    while(temp) {
      fread(&temp->current_items, sizeof(temp->current_items), 1, map_file);
      temp = temp->next;
    }
  }

  for (int k=0; k < map->numVertices; k++) {
    map->adjLists[k]->allocated_items[0] = map->adjLists[k]->allocated_items[1] = -1; // allocated_items = -1 means there are no assigned items yet
  }

  for (int i = 0; i < map->numItems; i++) { // Assigning items to rooms
    item temp = map->items[i];
    if (map->adjLists[temp.allocated_to_room]->allocated_items[0] == -1) map->adjLists[temp.allocated_to_room]->allocated_items[0] = temp.item_number;
    else if (map->adjLists[temp.allocated_to_room]->allocated_items[1] == -1) map->adjLists[temp.allocated_to_room]->allocated_items[1] = temp.item_number;
  }

  fclose(map_file);

  return map;
}

// --------------------------------- Game functions ---------------------------------

void kThreadMethod(graph *gr, int threadAmount, int from_room, int to_room) {

  if (from_room == to_room) {printf("\nBut you are already in the room %d\n", from_room); return;}

  if (to_room < 0 || to_room >= gr->numVertices) {printf("There is no such room on the map: %d", to_room); return;}

  path *paths = malloc(sizeof(path) * threadAmount);
  path *min_path = malloc(sizeof(path));
  min_path->from_room = from_room;
  min_path->to_room = to_room;
  min_path->path_length = -1;
  unsigned int minimum_path_length = 0;
  unsigned int *possible_path_length;

  if (paths == NULL) ERR("Error malloc for paths!\n");
  srand(time(NULL));
  for (int i = 0; i < threadAmount; i++) {
    paths[i].seed = rand();
    paths[i].map = gr;
    paths[i].from_room = from_room;
    paths[i].to_room = to_room;
  }

  for (int i = 0; i < threadAmount; i++) {
    int err = pthread_create(&(paths[i].tid), NULL, findPath, &paths[i]);
    if (err != 0) ERR("Couldn't create thread");
  }

  for (int i = 0; i < threadAmount; i++) {
    int err = pthread_join(paths[i].tid, (void*)&possible_path_length);
    if (err != 0) ERR("Couldn't join thread");

    if (minimum_path_length == 0) { // Initially minimum_path_length = 0, so this block of code just storing first path, that comes from threads
      minimum_path_length = *possible_path_length;
      min_path->path_length = minimum_path_length;
      for (int j = 0; j < minimum_path_length; j++) {
        min_path->path_rooms[j] = paths[i].path_rooms[j];
      }
    }

    else if (*possible_path_length < minimum_path_length && *possible_path_length > 0) { // If any other path is shorter, then it will become the shortest path
      minimum_path_length = *possible_path_length;
      min_path->path_length = minimum_path_length;
      for (int j = 0; j < minimum_path_length; j++) {
        min_path->path_rooms[j] = paths[i].path_rooms[j];
      }
    }

    if (possible_path_length != NULL) free(possible_path_length);

  }
  free(paths);

  if (min_path->path_length > gr->numVertices) { // Obviously, in the connected graph, shortest path between any two points is at most [number of vertices - 1]
    printf("\nPath from %d to %d:\n\n", gr->play.currentRoom, to_room);
    for (int i = 0; i < min_path->path_length; i++) {
    printf("%d ", min_path->path_rooms[i]);
    }
    printf("\n\nBut it is definitely not a good path! Length of the path [%d] should not be greater than the number of rooms [%d] in the map. So retype the command, with more than %d threads.\n", min_path->path_length, gr->numVertices, threadAmount);
    }

  else {
    printf("\nPath from %d to %d using %d-threads:\n\n", gr->play.currentRoom, to_room, threadAmount);

  for (int i = 0; i < min_path->path_length; i++) {
    printf("%d ", min_path->path_rooms[i]);
  }
  printf("\n");
  }
  free(min_path);
}

void *findPath(void *voidPtr) {
  path *current_path = voidPtr;
  int *pos_leng = malloc(sizeof(int));
  int length_count = 0;
  int cur_room = current_path->from_room;

  pthread_mutex_t mx_map = PTHREAD_MUTEX_INITIALIZER;
  current_path->mx_map = &mx_map;
  pthread_mutex_lock(&mx_map);
  for (int i = 0; i < 1000; i++) {
    if (cur_room == current_path->to_room) { *pos_leng = (int)length_count; return pos_leng; } // If we reach needed room, thread ends
    
    int *adj_rooms = malloc(sizeof(int) * numberAdjVertices(current_path->map, cur_room)); // Generating adjacent rooms
    room *temp = current_path->map->adjLists[cur_room];
    for (int j = 0; j < numberAdjVertices(current_path->map, cur_room); j++) {
      adj_rooms[j] = temp->vertex;
      temp = temp->next;
    }
    int rando = (rand_r(&current_path->seed) % numberAdjVertices(current_path->map, cur_room)); // Random number [0, number of adj rooms)
    
    cur_room = adj_rooms[rando]; // Moving into random adjacent room
    current_path->path_rooms[i] = cur_room;
    free(temp);
    free(adj_rooms);
    length_count++;
  }
  pthread_mutex_unlock(&mx_map);

  if (cur_room != current_path->to_room) { *pos_leng = (int)-1; return pos_leng;} // Returning -1, if the final room is not reached

  *pos_leng = (int)length_count; 
  return pos_leng;
}

void *autosave(void *arg) {
  graph *map = arg;
  while (1) {
    struct timespec sec = {30, 0};
    nanosleep(&sec, NULL);
    saveGame(map, map->backupPath);
  }

    return NULL;
}

void gameEnd(graph *gr) {
  gr->numCorrectItems = 0;
  for (int i = 0; i < gr->numVertices; i++) {
    room *temp = gr->adjLists[i];
    if ((temp->current_items[0] == temp->allocated_items[0] || temp->current_items[0] == temp->allocated_items[1]) && (temp->current_items[0] != -1)) {
      gr->numCorrectItems++;
    }
    if ((temp->current_items[1] == temp->allocated_items[0] || temp->current_items[1] == temp->allocated_items[1]) && (temp->current_items[1] != -1)) {
      gr->numCorrectItems++;
    }
  }
}

void *inGame(void *arg) {
  char *move_to = "move-to"; char *pick_up = "pick-up"; char *drop = "drop"; char *save = "save"; char *findPath = "find-path"; char *quit = "quit";

  graph *map = arg;

  char command[MAX_LINE+2];
  int main_menu_ = 0;


  while (map->numCorrectItems != map->numItems) {

      currentGameOutput(map);

      fgets(command, MAX_LINE+1, stdin); // Read one command

      if (strncmp(command, move_to, strlen(move_to)) == 0) {
        int to_the_room = singleIntFromLine(command, move_to);
        moveTo(map, to_the_room); 
      }
      else if (strncmp(command, pick_up, strlen(pick_up)) == 0) {
        int item_number = singleIntFromLine(command, pick_up);
        pickUp(map, item_number);
      }
      else if (strncmp(command, drop, strlen(drop)) == 0) {
        int item_number = singleIntFromLine(command, drop);
        dropItem(map, item_number);

      }
      else if (strncmp(command, save, strlen(save)) == 0) {
        char *path = malloc(sizeof(path));
        path = singlePathFromLine(command, save);
        printf("\n%s\n", path);
        saveGame(map, path);
      }
      else if (strncmp(command, findPath, strlen(findPath)) == 0) {
        char *threadsChar = pathDFromLine(command, findPath);
        char *toRoomChar = pathFFromLine(command, threadsChar, findPath);

        int threadsAmount = atoi(threadsChar);
        int toRoom = atoi(toRoomChar);

        kThreadMethod(map, threadsAmount, map->play.currentRoom, toRoom);
      }
      else if (strncmp(command, quit, strlen(quit)) == 0) {
        main_menu_ = 1;
        break;
      }
      else {
      printf("\nWrong command\n");
    }
    gameEnd(map);
  }

  pthread_cancel(map->threads[1]);
  if (map->numItems == map->numCorrectItems) { printf("\n----------GAME OVER----------\n"); mainMenuInstructions(); return(void*)1; }
  if (main_menu_ == 1) { mainMenuInstructions(); return (void*)2; }
  return (void*)0;
}

void moveTo(graph *map, int to) {
  if (isThereEdge(map, map->play.currentRoom, to) == 1) {
          map->play.currentRoom = to;
        }
  else {
          printf("You can't move from %d to %d\n", map->play.currentRoom, to);
        }
}

void pickUp(graph *map, int item_number) {
  if (map->adjLists[map->play.currentRoom]->current_items[0] == item_number) {
          if (map->play.current_items[0] == -1) { map->play.current_items[0] = item_number; map->adjLists[map->play.currentRoom]->current_items[0] = -1;}
          else if (map->play.current_items[1] == -1) { map->play.current_items[1] = item_number; map->adjLists[map->play.currentRoom]->current_items[0] = -1;} 
          else printf("You cannot carry more items :(\n");
        }
  else if (map->adjLists[map->play.currentRoom]->current_items[1] == item_number) {
          if (map->play.current_items[0] == -1) { map->play.current_items[0] = item_number; map->adjLists[map->play.currentRoom]->current_items[1] = -1; }
          else if (map->play.current_items[1] == -1) { map->play.current_items[1] = item_number; map->adjLists[map->play.currentRoom]->current_items[1] = -1; }
          else printf("You cannot carry more items :(\n");
        }
  else {
          printf("There is no such item in this room\n");
        }
}

void dropItem(graph *map, int item_number) {
  if ((map->adjLists[map->play.currentRoom]->current_items[0] != -1) && (map->adjLists[map->play.currentRoom]->current_items[1] != -1)) printf("\nThere is already maximum number of items in the room :(\n");

  else if (map->play.current_items[0] == item_number) {
          if (map->adjLists[map->play.currentRoom]->current_items[0] == -1) {
            map->adjLists[map->play.currentRoom]->current_items[0] = item_number;
            map->play.current_items[0] = -1;
          }
          else if (map->adjLists[map->play.currentRoom]->current_items[1] == -1) {
            map->adjLists[map->play.currentRoom]->current_items[1] = item_number;
            map->play.current_items[0] = -1;
          }
        }

  else if (map->play.current_items[1] == item_number) {
          if (map->adjLists[map->play.currentRoom]->current_items[0] == -1) {
            map->adjLists[map->play.currentRoom]->current_items[0] = item_number;
            map->play.current_items[1] = -1;
          }
          else if (map->adjLists[map->play.currentRoom]->current_items[1] == -1) {
            map->adjLists[map->play.currentRoom]->current_items[1] = item_number;
            map->play.current_items[1] = -1;
          }
        }

  else printf("You do not have such item!\n");  
}

// ----------------------------- Graph (map) functions --------------------------------

struct node* createNode(int v) {
  struct node* newNode = malloc(sizeof(struct node));
  newNode->vertex = v;
  newNode->next = NULL;
  return newNode;
}

struct Graph* createAGraph(int vertices) {
  struct Graph* graph = malloc(sizeof(struct Graph));
  graph->numVertices = vertices;
  graph->numItems = floor((double)(3*graph->numVertices)/2);
  graph->numCorrectItems = 0;

  graph->adjLists = malloc(vertices * sizeof(struct node*));

  int i;
  for (i = 0; i < vertices; i++)
    graph->adjLists[i] = NULL;

  return graph;
}

// Add edge
void addEdge(struct Graph* graph, int s, int d) {
  // Add edge from s to d
  struct node* newNode = createNode(d);
  newNode->next = graph->adjLists[s];
  graph->adjLists[s] = newNode;

  // Add edge from d to s
  newNode = createNode(s);
  newNode->next = graph->adjLists[d];
  graph->adjLists[d] = newNode;
}

int isThereEdge(graph *gr, int v1, int v2) { // If there as en edge between two rooms => player can move between them
  room *vert1 = gr->adjLists[v1];
  while (vert1) {
      if (vert1->vertex == v2) return 1;
      vert1 = vert1->next;
  }
  return 0;
}

int numberAdjVertices(graph *gr, int n) {
  int num = 0;
  room *temp = gr->adjLists[n];
  while (temp) {
    num++;
    temp = temp->next;
  }
  return num;
}

void printGraph(struct Graph* graph) {
  int v;
  for (v = 0; v < graph->numVertices; v++) {
    struct node* temp = graph->adjLists[v];
    int num = 0;
    printf("\n Room %d: ", v);
    printf("\n Items assigned to this room:");
    for (int k = 0; k < MAX_ITEMS; k++) {
      if (temp->allocated_items[k] != -1) printf("%d ", temp->allocated_items[k]);
    }
    printf("\n Current items: ");
    for (int k = 0; k < MAX_ITEMS; k++) {
      if (temp->current_items[k] != -1) printf("%d ", temp->current_items[k]);
    }
    printf("\n You can go to rooms:");
    while (temp) {
      num++;
      printf("%d , ", temp->vertex);
      temp = temp->next;
    }
    printf("\nAdj vertices: %d\n", num);
  }

  printf("Items on the map: ");
  for (int k = 0; k < graph->numItems; k++) {
    printf(" %d is allocated to the room %d", graph->items[k].item_number, graph->items[k].allocated_to_room);  
  }
}

// -------------------------- Main menu functions ---------------------------------

int mainMenu(char *backupPath) {
  char *read_map = "read-map"; char *map_from_dir_tree = "map-from-dir-tree"; char *generate_random_map = "generate-random-map"; char *load_game = "load-game"; char *exit = "exit";

  mainMenuInstructions();

  char command[MAX_LINE+2];

  while(fgets(command, MAX_LINE+1, stdin)) { // Read the command
      if (strncmp(command, read_map, strlen(read_map)) == 0) { // Determine what command was written using strncmp
        
        char *path = malloc(sizeof(path));
        strcpy(path, singlePathFromLine(command, read_map));

        readMap(path, backupPath);
        free(path);
      }
      else if (strncmp(command, map_from_dir_tree, strlen(map_from_dir_tree)) == 0) {

        char *path_d = pathDFromLine(command, map_from_dir_tree);
        char *path_f = pathFFromLine(command, path_d, map_from_dir_tree);

        mapFromDir(path_d, path_f);

        free(path_d);
        free(path_f);
      }
      else if (strncmp(command, generate_random_map, strlen(generate_random_map)) == 0) {

        char *n_of_rooms_string = pathDFromLine(command, generate_random_map);
        int n_of_rooms = atoi(n_of_rooms_string);
        char *path_f = pathFFromLine(command, n_of_rooms_string, generate_random_map);

        if (n_of_rooms <= 2) printf("\nThere must be more than 2 rooms\n");

        else {
          printf("\nCreated a map with %d rooms", n_of_rooms);
          randMap(n_of_rooms, path_f);
        }

        free(n_of_rooms_string);
        free(path_f);
      }
      else if (strncmp(command, load_game, strlen(load_game)) == 0) {
        char *path = singlePathFromLine(command, load_game);
        printf("\n%s123 \n%s123", backupPath, backupPath);
        startLoadGame(path, backupPath);
        // free(path);
      }
      else if (strncmp(command, exit, strlen(exit)) == 0) {
        return EXIT_SUCCESS;
      }
      else {
        printf("Wrong command\n");
      }
  }

  return EXIT_FAILURE;
}

void readMap(char *path, char *backupPath) {

  graph *gr = loadGame(path);

  startNewGame(gr, backupPath);
  
}

void randMap(int n, char *path) {
  
  srand(time(NULL)*getpid());

 graph *map = createAGraph(n);
 int arr[n];
 for (int j = 0; j < n; j++) { // Creating array consisting of [0, 1, 2, ... ,n - 1]
   arr[j] = j;
 }

  shuffle(arr, n);

  for (int j = 0; j < n - 1; j++) {
    addEdge(map, arr[j], arr[j+1]); // Adding edge between edges, such that all neighbors are connected. The graph will be in fact a path, so it will be 100% connected
  }
  int r1;
  int r2;

  for(int j = 0; j < ((rand() % n) - 1); j++) { // Creating random edges between random vertices
      r1 = ((rand() % n)); // randoming r1 [0, n) to create an edge r1-r2 
      r2 = ((rand() % n)); // randoming r2 [0, n) to create an edge r1-r2
      while (r2==r1 || isThereEdge(map, r1, r2) == 1) r2 = ((rand() % n)); // if r2==r1 or there is already an edge r2-r1 => r2 will random again
      addEdge(map, r1, r2); 
  }

  map->items = malloc(sizeof(item) * map->numItems);

  int room_arr[map->numVertices * 2];

  for (int k= 0, j = 0; k < map->numVertices * 2; k++, k++, j++) { // Creating array of rooms, which will look like [0, 0, 1, 1, 2, 2 ... n, n] 
      room_arr[k] = room_arr[k+1] = j;
  }

  for (int k=0; k < map->numVertices; k++) {
    map->adjLists[k]->current_items[0] = map->adjLists[k]->current_items[1] = -1; // current_item = -1 means there is no items
    map->adjLists[k]->allocated_items[0] = map->adjLists[k]->allocated_items[1] = -1; // allocated_items = -1 means there are no assigned items yet
  }

  shuffle(room_arr, map->numVertices * 2); // Shuffling the array, so now this array contains random rooms with no more than 2 similar rooms 
  
  for (int j = 0, t = 100; j < map->numItems; j++, t++) {
      map->items[j].item_number = t; // Each item must have the unique number from set [100, inf)
      map->items[j].allocated_to_room = room_arr[j]; // Each item have its own assigned room, which is taken from random [room_arr]
      
      if (map->adjLists[room_arr[j]]->allocated_items[0] == -1) { // Also adding the information about assigned items to room structure
        map->adjLists[room_arr[j]]->allocated_items[0] = map->items[j].item_number;
      }
      else if (map->adjLists[room_arr[j]]->allocated_items[1] == -1) {
        map->adjLists[room_arr[j]]->allocated_items[1] = map->items[j].item_number;
      }
  }


  int item_arr[map->numItems]; // Creating the array of all items on the map

  for (int k=0; k<map->numItems; k++) {
    item_arr[k] = map->items[k].item_number; // Copying values
  }

  shuffle(item_arr, map->numItems); // Shuffling the array of items

  for (int j = 0, k = 0; k < map->numItems; j++) { // Assigning each room current item. Items are taken from randomly created array item_arr 
    if (j == map->numVertices) { j = 0; } 
    int zero_or_one = ((rand() % 2));
    if (zero_or_one == 0) {
        continue;
    }
    else {
      if (map->adjLists[j]->allocated_items[0] == item_arr[k] || map->adjLists[j]->allocated_items[1] == item_arr[k]) {continue;} // Checking, that items won't spawn initially in the correct room 
      if (map->adjLists[j]->current_items[0] == -1) { map->adjLists[j]->current_items[0] = item_arr[k]; k++; }
      else if (map->adjLists[j]->current_items[1] == -1) { map->adjLists[j]->current_items[1] = item_arr[k]; k++; }
      else continue;
    }
  }

  saveGame(map, path);
 
}

void startNewGame(graph *map, char *backupPath) {
  gameInstructions();
  map->play.current_items[0] = map->play.current_items[1] = -1; // Player starts with no items
  map->play.currentRoom = (rand() % map->numVertices); // Random staring room
  gameEnd(map); // Initialazing current correct items (must be 0 initially)
  
  map->backupPath = malloc(sizeof(char) * strlen(backupPath) + 1);
  strcpy(map->backupPath, backupPath);

  if (pthread_create(&map->threads[0], NULL, inGame, map)) ERR("\nFailed to create a thread\n");
  if (pthread_create(&map->threads[1], NULL, autosave, map)) ERR("\nFailed to create a thread\n");

  if (pthread_join(map->threads[0], NULL)) ERR("\nFailed to join a thread!\n");
  if (pthread_join(map->threads[1], NULL)) ERR("\nFailed to join a thread!\n");


  //inGame(map); // Go to the game
}

void mapFromDir(char *pathd, char *pathf) {
    int number_of_rooms = 0;
    
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) { printf("Failed to get current directory path\n"); return; } // Save inital directory path

    if(nftw(pathd,walk,MAXFD,FTW_PHYS)==0) {
        number_of_rooms = global_dir_number; // Total number of subdirectories = total number of rooms in the map
        global_dir_number = 0;
    }

    graph *map = createAGraph(number_of_rooms);
    int currentRoomNumber = 1; // Assume, there is already one room, which is given [*pathd], so next room must have number [1]

    recursiveCreateMap(map, pathd, 0, &currentRoomNumber);

    chdir(cwd); // Returning to the initial directory (because recursive function above ends in some unpredictable directory)

    map->items = malloc(sizeof(item) * map->numItems);

    int room_arr[map->numVertices * 2];

  for (int k= 0, j = 0; k < map->numVertices * 2; k++, k++, j++) { // Creating array of rooms, which will look like [0, 0, 1, 1, 2, 2 ... n, n] 
      room_arr[k] = room_arr[k+1] = j;
  }

  for (int k=0; k < map->numVertices; k++) {
    map->adjLists[k]->current_items[0] = map->adjLists[k]->current_items[1] = -1; // current_item = -1 means there is no items
    map->adjLists[k]->allocated_items[0] = map->adjLists[k]->allocated_items[1] = -1; // allocated_items = -1 means there are no assigned items yet
  }

  shuffle(room_arr, map->numVertices * 2); // Shuffling the array, so now this array is random rooms with no more than 2 similar rooms 
  
  for (int j = 0, t = 100; j < map->numItems; j++, t++) {
      map->items[j].item_number = t; // Each item must have the unique number from set [100, inf)
      map->items[j].allocated_to_room = room_arr[j]; // Each item have its own assigned room, which is taken from random [room_arr]
      
      if (map->adjLists[room_arr[j]]->allocated_items[0] == -1) { // Also adding the information about assigned items to room structure
        map->adjLists[room_arr[j]]->allocated_items[0] = map->items[j].item_number;
      }
      else if (map->adjLists[room_arr[j]]->allocated_items[1] == -1) {
        map->adjLists[room_arr[j]]->allocated_items[1] = map->items[j].item_number;
      }
  }

  int item_arr[map->numItems]; // Creating the array of all items on the map

  for (int k=0; k<map->numItems; k++) {
    item_arr[k] = map->items[k].item_number; // Copying values
  }

  shuffle(item_arr, map->numItems); // Shuffling the array of items

  for (int j = 0, k = 0; k < map->numItems; j++) { // Assigning each room current item. Items are taken from randomly created array item_arr    
    if (j == map->numVertices) { j = 0; } 
    int zero_or_one = ((rand() % 2));
    if (zero_or_one == 0) {
        continue;
    }
    else {
        if (map->adjLists[j]->current_items[0] == -1) { map->adjLists[j]->current_items[0] = item_arr[k]; k++; }
        else if (map->adjLists[j]->current_items[1] == -1) { map->adjLists[j]->current_items[1] = item_arr[k]; k++; }
        else continue;
    }
  }
  
  printf("Created a map with [%d] rooms", map->numVertices);
   saveGame(map, pathf);


} 

void startLoadGame(char *path, char *backupPath) {
  gameInstructions();
  graph *map = loadGame(path);
  map->backupPath = malloc(sizeof(char) * strlen(backupPath) + 1);
  strcpy(map->backupPath, backupPath);

  if (pthread_create(&map->threads[0], NULL, inGame, map)) ERR("\nFailed to create a thread\n");
  if (pthread_create(&map->threads[1], NULL, autosave, map)) ERR("\nFailed to create a thread\n");

  if (pthread_join(map->threads[0], NULL)) ERR("\nFailed to join a thread!\n");
  if (pthread_join(map->threads[1], NULL)) ERR("\nFailed to join a thread!\n"); 
}

void recursiveCreateMap(graph *map, char *path, int roomNumber, int *currentRooms) {
  if (*currentRooms > map->numVertices) return; // Stop condition
  if (chdir(path)) { ERR("Cannot open this path");}
  
  int numberSub = currentDirectorySubfolders() - 2; // Subtracting 2, because all directories have "." and ".." as subdirectories, and we do not need them
  int curRoomsArr[numberSub];

  for (int i = 0; i < numberSub; i++) {
    if (isThereEdge(map, roomNumber, *currentRooms) == 0) {
      addEdge(map, roomNumber, *currentRooms);
       curRoomsArr[i] = *currentRooms; // Saving all subdirectories (rooms) IDs
      *currentRooms += 1;
    }
  }

  DIR *folder;
  struct dirent *entry;
  struct stat filestat;

  folder = opendir(".");
  if(folder == NULL)
    {
      ERR("Unable to read directory");
    }

  while ((strcmp(readdir(folder)->d_name, "..") == 0) || (strcmp(readdir(folder)->d_name, ".") == 0)) {entry = readdir(folder);} // Ensuring that entry is not "." or ".."

  for(int i = 0; i < numberSub; i++) 
  {
      entry=readdir(folder);
      stat(entry->d_name,&filestat);
      if( S_ISDIR(filestat.st_mode) ) {
        recursiveCreateMap(map, entry->d_name, curRoomsArr[i], currentRooms); // Entering each subdirectory
      }
  }
  chdir(".."); // Return to parent directory
  closedir(folder);
}

int walk(const char *name, const struct stat *s, int type, struct FTW *f)
{
	switch (type){
		case FTW_DNR:
		case FTW_D: global_dir_number++; break; 
	}
	return 0;
}

int currentDirectorySubfolders() {
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    int dirs=0;
    if(NULL==(dirp=opendir("."))) ERR("opendir");
    do {
        errno=0;
        if((dp=readdir(dirp))!=NULL) {
            if (lstat(dp->d_name, &filestat)) ERR("lstat");
            if (S_ISDIR(filestat.st_mode)) dirs++;
        }
    } while (dp!=NULL);
  
    if (errno!=0) ERR("readdir");
    if (closedir(dirp)) ERR("closedir");
    return dirs;
}

void shuffle(int *array, size_t n)
{
    
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + (rand() / ((RAND_MAX / (n - i)) + 1));
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}
