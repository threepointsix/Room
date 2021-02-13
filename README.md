# Room
 POSIX-complaint console game
 ## Requirements
 Use any POSIX-oriented operating system.
## Get ready
Dearchive all files into one folder and compile the program.
 ```bash
make main
```
 To run the program, execute:
 ```bash
./main
```
 ## How to start
 
 The aim of the game is to put all items in particular rooms.
 
 First you need to generate the map.
 
 * To generate a random map of *n* rooms and save the map as file *path*
 ```bash
 generate-random-map [n] [path]
 ```
 * To generate a map that mirrors a directory structure of *path-d* and save the map as file *path-f*
 ```bash
 map-from-dir-tree [path-d] [path-f]
 ```
 Now you can start a new game by reading file *path*
  ```bash
 read-map [path]
 ```
 
 ## How to play
 
 At the beginning, you will be spawned in random room.
 
 After each move the console will output the game state:
 ```
 Total number of items on map: ...
 Total number of items in correct spots: ...
 You are currently in the room: ...
 You can travel to the rooms: ...
 Your current items: ...
 Items in this room: ...
 Items assigned to this room: ...
 ```
You should pick up and drop the items in correct rooms:
 ```bash
 pick-up [y]
 drop [z]
 ```
 You can move to the adjacent rooms:
 ```bash
 move-to [x]
 ```
 ## Looking for path
 If you want to find the shortest path to some room *x*:
  ```bash
find-path [k] [x]
 ```
 Find-path algorithm uses *k*-thread method: program will start *k* parallel threads, each thread will try to find a path to *x*, and the shortest path among all threads will be presented to the player. 
 ## Save and load
 
 The whole game state (current room, items location, items assignment) is autosaved each 60 seconds:
  ```bash
 Saved the current game to the [/home/user/game-autosave]
 ```
 The current game can be manually saved to *path* and loaded from *path*:
  ```bash
 save [path]
 load-game [path]
 ```
 
