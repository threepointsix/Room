#define _XOPEN_SOURCE 500
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

int num_of_subfolders(char *path);
void game();
void rand_map(int n, char *path);

int sub_num = 0; int node_num = 1;

typedef struct item {
  int item_number;
  int allocated_to_room;
} item;

typedef struct player {
  item current_items[MAX_ITEMS];
} player;

typedef struct node {
  int vertex;
  item current_items[MAX_ITEMS];
  struct node* next;
} room;

struct node* createNode(int);

typedef struct Graph {
  int numVertices;
  int numItems;
  int numCorrectItems;
  player play;
  struct node** adjLists;
} graph;

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
    printf("\n Vertex %d: ", v);
    while (temp) {
      printf("%d -> ", temp->vertex);
      temp = temp->next;
    }
    printf("\n");
  }
}

int isThereEdge(graph *gr, int v1, int v2) {
  room *vert1 = gr->adjLists[v1];
  while (vert1) {
      if (vert1->vertex == v2) return 1;
      vert1 = vert1->next;
  }

  return 0;
  
}

void usage() {
    fprintf(stderr, "USAGE: ./main [-b]\n");
    fprintf(stderr, "b: optional backup-path\n");
}

/* int walk(const char *name, const struct stat *s, int type, struct FTW *f)
{
	switch (type){
		case FTW_DNR:
		case FTW_D: dirs++; break; 
	}
	return 0;
} */

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
}

struct Graph generate_graph(const char *pathd, const char *pathf) {
    int number_of_rooms = 0;

    if(nftw(pathd,walk,MAXFD,FTW_PHYS)==0) {
        number_of_rooms = dirs;
        dirs = 0;
    }

    struct Graph* map = createAGraph(number_of_rooms);
    int k = 1;
    createNode(k);
    k++;

    for (int i = 1; i < number_of_rooms; i++) {
        for (int j = 0; )
    }

} */

int main_menu() {
  char *read_map = "read-map";
  char *map_from_dir_tree = "map-from-dir-tree";
  char *generate_random_map = "generate-random-map";
  char *load_game = "load-game";
  char *exit = "exit";

  printf("\n---------------Main menu---------------\n\n");
  printf("read-map [path]\n");
  printf("    Read a map from a file given as an agrument and starts a new game on it.\n\n");
  printf("map-from-dir-tree [path-d] [path-f]\n");
  printf("    Generates a map that mirrors a directory structure of path-d and its subdirectories.\n\n");
  printf("generate-random-map [n] [path]\n");
  printf("    Generates a random map of n rooms and writes the result in path.\n\n");
  printf("load-game [path]\n");
  printf("    Loads the game state from path.\n\n");
  printf("exit\n");
  printf("    Exit the game.\n\n");

  char command[MAX_LINE+2];
  while(fgets(command, MAX_LINE+1, stdin)) {
      if (strncmp(command, read_map, strlen(read_map)) == 0) { // Reading the command
        char path[MAX_LINE - strlen(command)];
        for (int i = strlen(read_map) + 1, j = 0; i < strlen(command) - 1; i++, j++) { // Storing path as a string
            path[j] = command[i];
        }
        printf("%s %lu\n", path, strlen(path));
        game();
      }
      else if (strncmp(command, map_from_dir_tree, strlen(map_from_dir_tree)) == 0) {
        char path_d[MAX_LINE - strlen(command)];
        char path_f[MAX_LINE - strlen(command)];
        for (int i = strlen(map_from_dir_tree) + 1, j = 0; i < strlen(command); i++, j++) {
            if (command[i] == ' ') break;
            path_d[j] = command[i];
        }

        for (int i = strlen(map_from_dir_tree) + strlen(path_d) + 2, j = 0; i < strlen(command) - 1; i++, j++) {
            path_f[j] = command[i];
        }

        printf("path-d: %s %lu, path-f: %s %lu\n", path_d, strlen(path_d), path_f, strlen(path_f));
      }
      else if (strncmp(command, generate_random_map, strlen(generate_random_map)) == 0) {
        char number_of_rooms[MAX_LINE - strlen(command)];
        int n_of_rooms;
        char path[MAX_LINE - strlen(command)];
        for (int i = strlen(generate_random_map) + 1, j = 0; i < strlen(command); i++, j++) {
            if (command[i] == ' ') break;
            number_of_rooms[j] = command[i];
        }
        n_of_rooms = atoi(number_of_rooms);
        for (int i = strlen(generate_random_map) + strlen(number_of_rooms) + 2, j = 0; i < strlen(command) - 1; i++, j++) {
            path[j] = command[i];
        }
        
        rand_map(n_of_rooms, path);
        printf("Number of rooms: %d, path: %s \n", n_of_rooms, path);
      }
      else if (strncmp(command, load_game, strlen(load_game)) == 0) {
        char path[MAX_LINE - strlen(command)];
        for (int i = strlen(load_game) + 1, j = 0; i < strlen(command) - 1; i++, j++) {
            path[j] = command[i];
        }
        printf("%s %lu\n", path, strlen(path));
      }
      else if (strncmp(command, exit, strlen(exit)) == 0) {
        return EXIT_SUCCESS;
      }
      else {
        printf("Wrong command");
      }
  }

  return EXIT_FAILURE;
}

void game() {
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
}

void rand_map(int n, char *path) {
 graph *map = createAGraph(n);
 int i;
 for (i = 0; i < n; i++) {
   addEdge(map, i, i+1);
 }
  addEdge(map, 0, i); // Creating a circuit in the graph, so it will be 100% connected

  srand(time(NULL));

  int r1;
  int r2;

  for(i = 0; i < ((rand() % n) + 1); i++) { // Creating random edges between random vertices
      r1 = ((rand() % n)); // randoming r1 [0, n) to create an edge r1-r2 
      r2 = ((rand() % n)); // randoming r2 [0, n) to create an edge r1-r2
      while (r2==r1 || isThereEdge(map, r1, r2) == 1) r2 = ((rand() % n) + 1); // if r2==r1 or there is already an edge r2-r1 => r2 will random again
      addEdge(map, r1, r2); 
  }

  printGraph(map);
}

int main(int argc, char** argv) {
    if (argc > 2) usage();

    main_menu();

    struct Graph* graph = createAGraph(4);
    addEdge(graph, 0, 1);
    addEdge(graph, 0, 2);
    addEdge(graph, 0, 3);
    addEdge(graph, 1, 2);

    printGraph(graph);

    /*if(nftw(argv[1],walk,MAXFD,FTW_PHYS)==0)
			printf("Number of subfolders: %d\n", dirs);

    printf("Number of subfolders: %d\n", num_of_subfolders(argv[1])); */



    return EXIT_SUCCESS;
}
