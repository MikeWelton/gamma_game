## Gamma

Gamma is a simple terminal game taking place on a rectangle board.

### Rules

Board is split into fields. Main target for every player is to obtain as many fields as possible.
Adjacent fields belonging to player create area. Each player can only own some specified maximum number of areas (it is one of the game parameters).
Fields can possesed by making moves onto them. Each player makes one move per turn. 
Move can be made only onto free fields (not belonging to anybody). Every player once per game can make "golden move" which allows him to move onto busy (other player's) field.
Move cannot be made if it would add new area above limit or split another player's areas creating additional area above limit.
Game ends when nobody can make move anymore (all fields are busy or every next move violates areas limit).

### Game start

Program containing gamma game works in 2 modes:
1) batch
2) interactive

At the begining program expects one of two commands:

`sh
B width height players areas` to enter batch mode or

`sh
I width height players areas` to enter interactive mode

These commands are creating new game calling function gamma_new. 
In interactive mode after successful creation board will appear whereas in batch mode it will be confirmed by prompt "OK line_num".

### Interactive mode

In interactive mode move with ARROWS, make moves with SPACE and golden move with G. To exit game click Ctrl-D. 

### Batch mode

In batch mode, program accepts commands:

`m player x y` – calling function gamma_move,

`g player x y` – calling function gamma_golden_move,

`b player` – calling function gamma_busy_fields,

`f player` – calling function gamma_free_fields,

`q player` – calling function gamma_golden_possible,

`p` – calling function gamma_board.

Every invalid command is followed with information about error. From batch mode it is still possible to access interactive mode.
To exit game click Ctrl-D. 

### Compilation

On Linux compile with commands (after moving to project directory):
```sh
mkdir release
cd release
cmake ..
make
```
Start with:
```sh
./gamma
```

Rest of documentation for project is available in Polish in source files. Alternatively you can create docs with:
```sh
make
make doc
```

