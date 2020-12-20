#include <linux/limits.h>
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

#define MAXFD 20
#define MAX_LINE 100
#define MAX_ITEMS 2
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))



int global_dir_number = 0;

// Structures

typedef struct item {
  int item_number;
  int allocated_to_room;
} item;

typedef struct player {
  int current_items[MAX_ITEMS];
} player;

typedef struct node {
  int vertex;
  int allocated_items[MAX_ITEMS];
  int current_items[MAX_ITEMS];
  struct node *next;
} room;

typedef struct Graph {
  int numVertices;
  int numItems;
  int numCorrectItems;
  int currentRoom;
  player play;
  item *items;
  struct node** adjLists;
} graph;

typedef struct path {
  pthread_t tid;
  unsigned int seed;
  int from_room;
  int to_room;
  int path_length;
  int *path_rooms;
  graph *map;
} path;

// Declaring all functions

struct node* createNode(int);
int num_of_subfolders(char *path);
void game();
void rand_map(int n, char *path);
void read_map_f(char *path);
void start_game(graph *map);
void save_game(graph *map, char *path);
void game_end(graph *gr);
void start_load_game(char *path);
void main_menu_instructions();
void game_instructions();
void current_game_output(graph *gr);
char* single_path_from_command(char *line, char *command);
char* path_d_from_command(char *line, char *command);
char* path_f_from_command(char *line, char *path_d, char *command);
int int_from_command(char *line, char *command);
graph *load_game_func(char *path);
path k_thread_method(graph *gr, int threadAmount, int from_room, int to_room);
void *find_path(void *voidPtr);
int currentDirectorySubfolders();
void mapFromDir(char *pathd, char *pathf);
void recursiveCreateMap(graph *map, char *path, int roomNumber, int *currentRooms);
void shuffle(int *array, size_t n);

// Create a node
struct node* createNode(int v) {
  struct node* newNode = malloc(sizeof(struct node));
  newNode->vertex = v;
  newNode->next = NULL;
  return newNode;
}

// Create a graph
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

// Print the graph
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

void usage() {
    fprintf(stderr, "USAGE: ./main [-b]\n");
    fprintf(stderr, "b: optional backup-path\n");
}

 int walk(const char *name, const struct stat *s, int type, struct FTW *f)
{
	switch (type){
		case FTW_DNR:
		case FTW_D: global_dir_number++; break; 
	}
	return 0;
}

int num_of_subfolders(char *path) {

  int file_count = 0;
  DIR * dirp;
  struct dirent * entry;

  dirp = opendir(path); /* There should be error handling after this */
  while ((entry = readdir(dirp)) != NULL) {
    
}
  closedir(dirp);
  return file_count;
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

/* void scan_dir (char *path, int total_subfolders) {
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;

    int folders_subfolders[total_subfolders];
    char *npath[total_subfolders];

	int dirs=0;
	if (NULL == (dirp = opendir(path))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");
			if (S_ISDIR(filestat.st_mode)) {
                dirs++;
                npath=malloc(strlen(path)+strlen(dp->d_name)+2);
                sprintf(npath,"%s/%s",path, dp->d_name);

            }
		}
	} while (dp != NULL);

	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
	printf("Dirs: %d\n",dirs);
} */

void mapFromDir(char *pathd, char *pathf) {
    int number_of_rooms = 0;

    if(nftw(pathd,walk,MAXFD,FTW_PHYS)==0) {
        number_of_rooms = global_dir_number;
        global_dir_number = 0;
    }
    // printf("%d", number_of_rooms);
    graph *map = createAGraph(number_of_rooms);
    int currentRoomNumber = 1;

    recursiveCreateMap(map, pathd, 0, &currentRoomNumber);

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

//    printGraph(map);


   save_game(map, pathf);

} 

void recursiveCreateMap(graph *map, char *path, int roomNumber, int *currentRooms) {
  if (*currentRooms > map->numVertices) return;
  if (chdir(path)) { ERR("Cannot open this path");}
  
  int numberSub = currentDirectorySubfolders() - 2;
  // printf("%d", numberSub);
//  char **subdirNames;

  // if (numberSub == 0) {  return; }

  int curRoomsArr[numberSub];

  for (int i = 0; i < numberSub; i++) {
    if (isThereEdge(map, roomNumber, *currentRooms) == 0) {
      addEdge(map, roomNumber, *currentRooms);
       curRoomsArr[i] = *currentRooms;
/*        printf("\nAdded edge between %d and %d", roomNumber, *currentRooms);
       printf("%d", *currentRooms); */
      *currentRooms = *currentRooms + 1;
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

/*   if (numberSub==0) printf(" 0 ");

  for (int i = 0; i < numberSub; i++) {
    printf(" %d ", curRoomsArr[i]);
  } */

  entry=readdir(folder);
  entry=readdir(folder);

  for(int i = 0; i < numberSub; i++) 
  {
      entry=readdir(folder);
      // printf("%s", entry->d_name);
      stat(entry->d_name,&filestat);
      if( S_ISDIR(filestat.st_mode) ) {
/*         printf("%s", entry->d_name);
        printf("%d", curRoomsArr[i]);
        printf("%d", *currentRooms); */
        recursiveCreateMap(map, entry->d_name, curRoomsArr[i], currentRooms);
        /* subdirNames[i] = malloc(sizeof(char) * strlen(entry->d_name));
        strcpy(subdirNames[i], entry->d_name); */
      }
  }
  chdir("..");
  closedir(folder);
}

int main_menu() {
  char *read_map = "read-map"; char *map_from_dir_tree = "map-from-dir-tree"; char *generate_random_map = "generate-random-map"; char *load_game = "load-game"; char *exit = "exit";

  main_menu_instructions();

  char command[MAX_LINE+2];

  while(fgets(command, MAX_LINE+1, stdin)) { // Read the command
      if (strncmp(command, read_map, strlen(read_map)) == 0) { // Determine what command was written using strncmp
        
        char *path = malloc(sizeof(path));
        strcpy(path, single_path_from_command(command, read_map));

/*         char path[MAX_LINE - strlen(command)];
        for (int i = strlen(read_map) + 1, j = 0; i < strlen(command) - 1; i++, j++) { // Storing path as a string
            path[j] = command[i];
        } */

        read_map_f(path);
        free(path);
      }
      else if (strncmp(command, map_from_dir_tree, strlen(map_from_dir_tree)) == 0) {

/*         char path_d[MAX_LINE - strlen(command)];
        char path_f[MAX_LINE - strlen(command)];
        for (int i = strlen(map_from_dir_tree) + 1, j = 0; i < strlen(command); i++, j++) {
            if (command[i] == ' ') break;
            path_d[j] = command[i];
        }

        for (int i = strlen(map_from_dir_tree) + strlen(path_d) + 2, j = 0; i < strlen(command) - 1; i++, j++) {
            path_f[j] = command[i];
        } */

        char *path_d = malloc(sizeof(path_d));
        char *path_f = malloc(sizeof(path_f));
        strcpy(path_d, path_d_from_command(command, map_from_dir_tree));
        strcpy(path_f, path_f_from_command(command, path_d, map_from_dir_tree));

        mapFromDir(path_d, path_f);
/*         printf("command length: %lu, map-from-dir-tree: %lu", strlen(command), strlen(map_from_dir_tree));
        printf("path-d: %s %lu, path-f: %s %lu\n", path_d, strlen(path_d), path_f, strlen(path_f));
        free(path_d);
        free(path_f); */
      }
      else if (strncmp(command, generate_random_map, strlen(generate_random_map)) == 0) {

/*         char number_of_rooms[MAX_LINE - strlen(command)];
        int n_of_rooms;
        char path[MAX_LINE - strlen(command)];
        for (int i = strlen(generate_random_map) + 1, j = 0; i < strlen(command); i++, j++) {
            if (command[i] == ' ') break;
            number_of_rooms[j] = command[i];
        }
        n_of_rooms = atoi(number_of_rooms);
        for (int i = strlen(generate_random_map) + strlen(number_of_rooms) + 2, j = 0; i < strlen(command) - 1; i++, j++) {
            path[j] = command[i];
        } */

        char *n_of_rooms_string = malloc(sizeof(n_of_rooms_string));
        strcpy(n_of_rooms_string, path_d_from_command(command, generate_random_map));
        int n_of_rooms = int_from_command(command, generate_random_map);
        char *path_f = malloc(sizeof(path_f));
        strcpy(path_f, path_f_from_command(command, n_of_rooms_string, generate_random_map));

        rand_map(n_of_rooms, path_f);
        free(n_of_rooms_string);
        free(path_f);
      }
      else if (strncmp(command, load_game, strlen(load_game)) == 0) {
        char *path = malloc(sizeof(path));
        strcpy(path, single_path_from_command(command, load_game));
        start_load_game(path);
        free(path);
//        printf("%s %lu\n", path, strlen(path));
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

/* void game() {
  char *move_to = "move-to";
  char *pick_up = "pick-up";
  char *drop = "drop";
  char *save = "save";
  char *find_path = "find-path";
  char *quit = "quit";

  printf("\n---------------Game---------------\n\n");
  printf("move-to [x]\n\n");
  printf("pick-up [y]\n\n");
  printf("drop [z]\n\n");
  printf("save [path]\n");
  printf("    Save a current game to the file\n\n");
  printf("find-path [k] [x]\n");
  printf("    Find a shortest path to x using k threads\n\n");
  printf("quit\n\n");

  char command[MAX_LINE+2];
  while(fgets(command, MAX_LINE+1, stdin)) {
    if (strncmp(command, move_to, strlen(move_to)) == 0) {
      printf("move_to");
    }
    else if (strncmp(command, pick_up, strlen(pick_up)) == 0) {
      printf("pick_up");
    }
    else if (strncmp(command, drop, strlen(drop)) == 0) {
      printf("drop");
    }
    else if (strncmp(command, save, strlen(save)) == 0) {
      printf("save");
    }
    else if (strncmp(command, find_path, strlen(find_path)) == 0) {
      printf("find_path");
    }
    else if (strncmp(command, quit, strlen(quit)) == 0) {
      printf("quit");
    }
    else {
      printf("Wrong command");
    }
  }
} */

void read_map_f(char *path) {

  graph *gr = load_game_func(path);

/*   printf("Vertices: %d\nItems: %d\nCorrect Items: %d\nCurrent Room: %d\n", gr->numVertices, gr->numItems, gr->numCorrectItems, gr->currentRoom);

  printf("Player items: %d %d\n", gr->play.current_items[0], gr->play.current_items[1]);

  for (int i = 0; i < gr->numItems; i++) {
    printf("Item: %d allocated to room %d\n", gr->items[i].item_number, gr->items[i].allocated_to_room);
  } */

  start_game(gr);

//   printGraph(gr); 
  
}

void start_game(graph *map) {
  
  char *move_to = "move-to"; char *pick_up = "pick-up"; char *drop = "drop"; char *save = "save"; char *find_path = "find-path"; char *quit = "quit";

  game_instructions();
  
  map->play.current_items[0] = map->play.current_items[1] = -1; // Player starts with no items
  
  char command[MAX_LINE+2];

  srand(time(NULL));

  map->currentRoom = (rand() % map->numVertices); // Random staring room

  int main_menu_ = 0;

  game_end(map);

  while (map->numCorrectItems != map->numItems) {

      current_game_output(map);

      fgets(command, MAX_LINE+1, stdin); // Read one command

      if (strncmp(command, move_to, strlen(move_to)) == 0) {
        char to_the_room_string[MAX_LINE - strlen(command)];
        int to_the_room;
        for (int i = strlen(move_to) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
          to_the_room_string[j] = command[i];
        }
        to_the_room = atoi(to_the_room_string);
        if (isThereEdge(map, map->currentRoom, to_the_room) == 1) {
          map->currentRoom = to_the_room;
        }
        else {
          printf("You can't move from %d to %d\n", map->currentRoom, to_the_room);
        }
      }
      else if (strncmp(command, pick_up, strlen(pick_up)) == 0) {
        char item_number_string[MAX_LINE - strlen(command)];
        int item_number;
        for (int i = strlen(move_to) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
          item_number_string[j] = command[i];
        }
        item_number = atoi(item_number_string);

        if (map->adjLists[map->currentRoom]->current_items[0] == item_number) {
          if (map->play.current_items[0] == -1) { map->play.current_items[0] = item_number; map->adjLists[map->currentRoom]->current_items[0] = -1;}
          else if (map->play.current_items[1] == -1) { map->play.current_items[1] = item_number; map->adjLists[map->currentRoom]->current_items[0] = -1;} 
          else printf("You cannot carry more items :(\n");
        }
        else if (map->adjLists[map->currentRoom]->current_items[1] == item_number) {
          if (map->play.current_items[0] == -1) { map->play.current_items[0] = item_number; map->adjLists[map->currentRoom]->current_items[1] = -1; }
          else if (map->play.current_items[1] == -1) { map->play.current_items[1] = item_number; map->adjLists[map->currentRoom]->current_items[1] = -1; }
          else printf("You cannot carry more items :(\n");
        }
        else {
          printf("There is no such item in this room\n");
        }
      }
      else if (strncmp(command, drop, strlen(drop)) == 0) {
        char item_number_string[MAX_LINE - strlen(command)];
        int item_number;
        for (int i = strlen(drop) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
          item_number_string[j] = command[i];
        }
        item_number = atoi(item_number_string);

        if ((map->adjLists[map->currentRoom]->current_items[0] != -1) && (map->adjLists[map->currentRoom]->current_items[1] != -1)) printf("\nThere is already maximum number of items in the room :(\n");

        else if (map->play.current_items[0] == item_number) {
          if (map->adjLists[map->currentRoom]->current_items[0] == -1) {
            map->adjLists[map->currentRoom]->current_items[0] = item_number;
            map->play.current_items[0] = -1;
          }
          else if (map->adjLists[map->currentRoom]->current_items[1] == -1) {
            map->adjLists[map->currentRoom]->current_items[1] = item_number;
            map->play.current_items[0] = -1;
          }
        }

        else if (map->play.current_items[1] == item_number) {
          if (map->adjLists[map->currentRoom]->current_items[0] == -1) {
            map->adjLists[map->currentRoom]->current_items[0] = item_number;
            map->play.current_items[1] = -1;
          }
          else if (map->adjLists[map->currentRoom]->current_items[1] == -1) {
            map->adjLists[map->currentRoom]->current_items[1] = item_number;
            map->play.current_items[1] = -1;
          }
        }

        else printf("You do not have such item!\n");

      }
      else if (strncmp(command, save, strlen(save)) == 0) {
        char *path = malloc(sizeof(path));
        path = single_path_from_command(command, save);
        
        save_game(map, path);

        printf("\n Saved the current game to the [%s]\n", path);
      }
      else if (strncmp(command, find_path, strlen(find_path)) == 0) {
        char *threadsChar = path_d_from_command(command, find_path);
        char *toRoomChar = path_f_from_command(command, threadsChar, find_path);

        int threadsAmount = atoi(threadsChar);
        int toRoom = atoi(toRoomChar);

        struct path min_path = k_thread_method(map, threadsAmount, map->currentRoom, toRoom);
        
        printf("Best path from %d to %d\nLength: %d\nPath:", map->currentRoom, toRoom, min_path.path_length);

        for (int i = 0; i < min_path.path_length; i++) {
            printf("%d ", min_path.path_rooms[i]);
        }
        printf("\n");
      }
      else if (strncmp(command, quit, strlen(quit)) == 0) {
        main_menu_ = 1;
        break;
      }
      else {
      printf("\nWrong command\n");
    }
    game_end(map);
  }

  if (map->numItems == map->numCorrectItems) printf("\n----------GAME OVER----------\n");
  if (main_menu_ == 1) main_menu();
}

void game_end(graph *gr) {
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

void start_load_game(char *path) {

  char *move_to = "move-to"; char *pick_up = "pick-up"; char *drop = "drop"; char *save = "save"; char *find_path = "find-path"; char *quit = "quit";

  game_instructions();

  graph *map = load_game_func(path);

  char command[MAX_LINE+2];
  int room_number = map->currentRoom;
  srand(time(NULL));
  int main_menu_ = 0;
  printf("%d", map->items[0].item_number);

  while (map->numCorrectItems != map->numItems) {
      
      current_game_output(map);

      fgets(command, MAX_LINE+1, stdin); // Read one command

      if (strncmp(command, move_to, strlen(move_to)) == 0) {
        char to_the_room_string[MAX_LINE - strlen(command)];
        int to_the_room;
        for (int i = strlen(move_to) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
          to_the_room_string[j] = command[i];
        }
        to_the_room = atoi(to_the_room_string);
        if (isThereEdge(map, room_number, to_the_room) == 1) {
          room_number = to_the_room;
          map->currentRoom = to_the_room;
        }
        else {
          printf("You can't move from %d to %d\n", room_number, to_the_room);
        }
      }
      else if (strncmp(command, pick_up, strlen(pick_up)) == 0) {
        char item_number_string[MAX_LINE - strlen(command)];
        int item_number;
        for (int i = strlen(move_to) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
          item_number_string[j] = command[i];
        }
        item_number = atoi(item_number_string);

        if (map->adjLists[room_number]->current_items[0] == item_number) {
          if (map->play.current_items[0] == -1) { map->play.current_items[0] = item_number; map->adjLists[room_number]->current_items[0] = -1;}
          else if (map->play.current_items[1] == -1) { map->play.current_items[1] = item_number; map->adjLists[room_number]->current_items[0] = -1;} 
          else printf("You cannot carry more items :(\n");
        }
        else if (map->adjLists[room_number]->current_items[1] == item_number) {
          if (map->play.current_items[0] == -1) { map->play.current_items[0] = item_number; map->adjLists[room_number]->current_items[1] = -1; }
          else if (map->play.current_items[1] == -1) { map->play.current_items[1] = item_number; map->adjLists[room_number]->current_items[1] = -1; }
          else printf("You cannot carry more items :(\n");
        }
        else {
          printf("There is no such item in this room\n");
        }
      }
      else if (strncmp(command, drop, strlen(drop)) == 0) {
        char item_number_string[MAX_LINE - strlen(command)];
        int item_number;
        for (int i = strlen(drop) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
          item_number_string[j] = command[i];
        }
        item_number = atoi(item_number_string);

        if ((map->adjLists[room_number]->current_items[0] != -1) && (map->adjLists[room_number]->current_items[1] != -1)) printf("\nThere is already maximum number of items in the room :(\n");

        else if (map->play.current_items[0] == item_number) {
          if (map->adjLists[room_number]->current_items[0] == -1) {
            map->adjLists[room_number]->current_items[0] = item_number;
            map->play.current_items[0] = -1;
          }
          else if (map->adjLists[room_number]->current_items[1] == -1) {
            map->adjLists[room_number]->current_items[1] = item_number;
            map->play.current_items[0] = -1;
          }
        }

        else if (map->play.current_items[1] == item_number) {
          if (map->adjLists[room_number]->current_items[0] == -1) {
            map->adjLists[room_number]->current_items[0] = item_number;
            map->play.current_items[1] = -1;
          }
          else if (map->adjLists[room_number]->current_items[1] == -1) {
            map->adjLists[room_number]->current_items[1] = item_number;
            map->play.current_items[1] = -1;
          }
        }

        else printf("You do not have such item!\n");

      }
      else if (strncmp(command, save, strlen(save)) == 0) {
        char path[MAX_LINE - strlen(command)];
        for (int i = strlen(save) + 1, j = 0; i < strlen(command) - 1; i++, j++) { // Storing path as a string
            path[j] = command[i];
        }
        
        FILE *map_file;

        map_file = fopen(path, "wb");

        if (map_file == NULL) {
          fprintf(stderr, "Error opening file\n");
        }
        fwrite(map, sizeof(graph), 1, map_file); // Writing the created map into the file [path]

        fclose(map_file);

        printf("\n Saved the current game to the [%s]\n", path);
      }
      else if (strncmp(command, find_path, strlen(find_path)) == 0) {


        printf("find_path");
      }
      else if (strncmp(command, quit, strlen(quit)) == 0) {
        main_menu_ = 1;
        break;
      }
      else {
      printf("\nWrong command\n");
    }
    game_end(map);
    printf("\nTotal number of items: %d \nTotal number of items in correct spots: %d\n", map->numItems, map->numCorrectItems);
  }
  if (map->numCorrectItems == map->numItems) printf("\n----------GAME OVER----------\n");
  if (main_menu_ == 1) main_menu();
}

void shuffle(int *array, size_t n)
{
    srand(time(NULL));

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

void rand_map(int n, char *path) {
  
 graph *map = createAGraph(n);
 int i;
 for (i = 0; i < n - 1; i++) {
   addEdge(map, i, i+1);
 }

  addEdge(map, 0, i); // Creating a circuit in the graph, so it will be 100% connected

  srand(time(NULL));

  int r1;
  int r2;

  for(i = 0; i < ((rand() % n) + 1); i++) { // Creating random edges between random vertices
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

//  printGraph(map);

  save_game(map, path);
  

//  printGraph(map);
//  printf("Vertices: %d, Items: %d, Correct Items: %d", map->numVertices, map->numItems, map->numCorrectItems);

}


int main(int argc, char** argv) {
    //if (argc > 2) usage();

    main_menu();

/*     struct Graph* graph = createAGraph(5);
    addEdge(graph, 0, 1);
    addEdge(graph, 0, 2);
    addEdge(graph, 0, 3);
    addEdge(graph, 0, 4);
    addEdge(graph, 1, 2);

    printGraph(graph); */

 /*
    if(nftw(argv[1],walk,MAXFD,FTW_PHYS)==0)
			printf("Number of subfolders: %d\n", dirs);

    printf("Number of subfolders: %d\n", num_of_subfolders(argv[1])); */



    return EXIT_SUCCESS;
}

// -------------------------- Output functions (not interesting) ------------------------

void main_menu_instructions() {
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

void game_instructions() {
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

void current_game_output(graph *map) {
  printf("\nTotal number of items: %d \nTotal number of items in correct spots: %d\n", map->numItems, map->numCorrectItems);

  printf("You are currently in the room: %d\n", map->currentRoom);
  printf("You can travel to the rooms:");
  room *temp = map->adjLists[map->currentRoom];
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
        if (map->adjLists[map->currentRoom]->current_items[k] != -1) printf("%d ", map->adjLists[map->currentRoom]->current_items[k]);
      }
      printf("\n");

      printf("Items assigned to this room:");
      for (int k = 0; k < MAX_ITEMS; k++) {
        if (map->adjLists[map->currentRoom]->allocated_items[k] != -1) printf("%d ", map->adjLists[map->currentRoom]->allocated_items[k]);
      }
      printf("\n");
}

// ------------------------ Functions for taking paths and integers from given commands -----------------------

char* single_path_from_command(char *line, char *command) {
  char *path = malloc(sizeof(path));
  int j = 0;
  for (int i = strlen(command) + 1; i < strlen(line) - 1; i++, j++) { // Storing path as a string
    path[j] = line[i];
  }
  path[j] = '\0'; 
  return path;
}

char* path_d_from_command(char *line, char *command) {
  char *path_d  = malloc(sizeof(path_d));
  int j = 0;
  for (int i = strlen(command) + 1; i < strlen(line); i++, j++) {
    if (line[i] == ' ') break;
    path_d[j] = line[i];
    }
  path_d[j] = '\0';
  return path_d;
}

char *path_f_from_command(char *line, char *path_d, char *command) {
  char *path_f = malloc(sizeof(path_f));
  int j = 0;
  for (int i = strlen(command) + strlen(path_d) + 2; i < strlen(line) - 1; i++, j++) {
    path_f[j] = line[i];
  }
  path_f[j] = '\0';
  return path_f;
}

int int_from_command(char *line, char *command) {
  char *num_string = malloc(sizeof(num_string));
  int num;
  strcpy(num_string, path_d_from_command(line, command));
  num = atoi(num_string);
  return num;
}

// ------------------------ Saving and loading game (marshalling) --------------------------


void save_game(graph *map, char *path) {
  FILE *map_file;

  map_file = fopen(path, "wb"); // Creating the file [path]

  if (map_file == NULL) {
     fprintf(stderr, "Error opening file\n");
  }

  fwrite(&map->numVertices, sizeof(int), 1, map_file);
  fwrite(&map->numItems, sizeof(int), 1, map_file);
  fwrite(&map->numCorrectItems, sizeof(int), 1, map_file);
  fwrite(&map->currentRoom, sizeof(int), 1, map_file);
  fwrite(&map->play, sizeof(player), 1, map_file);

  for (int i = 0; i < map->numItems; i++) {
    fwrite(&map->items[i], sizeof(map->items[i]), 1, map_file); 
  }

  for (int i = 0; i < map->numVertices; i++) {
    size_t length = numberAdjVertices(map, i);
    fwrite(&length, sizeof(length), 1, map_file);

    room *temp = map->adjLists[i];

    for(int j = 0; j < length; j++) {
      fwrite(&temp->vertex, sizeof(temp->vertex), 1, map_file);
      temp = temp->next;
    } 
  }

  for (int i = 0; i < map->numVertices; i++) {
    room *temp = map->adjLists[i];
    while(temp) {
      fwrite(&temp->current_items, sizeof(temp->current_items), 1, map_file);
      temp = temp->next;
    }
  }

  fclose(map_file);
}

graph *load_game_func(char *path) {
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
  fread(&map->currentRoom, sizeof(int), 1, map_file);
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

path k_thread_method(graph *gr, int threadAmount, int from_room, int to_room) {
  path *paths = malloc(sizeof(path) * threadAmount);
  path min_path;

  unsigned int minimum_path_length = gr->numVertices;
  unsigned int possible_path_length;

  if (paths == NULL) ERR("Error malloc for paths!\n");
  srand(time(NULL));

  for (int i = 0; i < threadAmount; i++) {
    paths[i].seed = rand();
    paths[i].map = gr;
    paths[i].from_room = from_room;
    paths[i].to_room = to_room;
  }

  for (int i = 0; i < threadAmount; i++) {
    int err = pthread_create(&(paths[i].tid), NULL, find_path, &paths[i]);
    if (err != 0) ERR("Couldn't create thread");
  }

  for (int i = 0; i < threadAmount; i++) {
    int err = pthread_join(paths[i].tid, (void*)&possible_path_length);
    if (err != 0) ERR("Couldn't create thread");

    if (possible_path_length < minimum_path_length) {
      minimum_path_length = possible_path_length;
      min_path.path_length = minimum_path_length;
      min_path.path_rooms = paths[i].path_rooms;
    }
  }
  free(paths);
  return min_path;
}

void *find_path(void *voidPtr) {
  path *current_path = voidPtr;
  int *pos_leng = 0;
  int cur_room = current_path->from_room;

  for (int i = 0; i < 1000; i++) {
    if (cur_room == current_path->to_room) return pos_leng;
    int *adj_rooms = malloc(sizeof(numberAdjVertices(current_path->map, cur_room)));
    room *temp = current_path->map->adjLists[cur_room];
    for (int j = 0; j < numberAdjVertices(current_path->map, cur_room); j++) {
      adj_rooms[j] = temp[j].vertex;
      temp = temp->next;
    }
    shuffle(adj_rooms, numberAdjVertices(current_path->map, cur_room));
    cur_room = adj_rooms[0];
    pos_leng++;
  }

  return pos_leng;
}
