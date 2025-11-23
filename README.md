# C-nnectFour (Console Connect Four in C)

A simple, console-based implementation of the classic Connect Four game written in C.

## Features

* Standard 7×6 Connect Four board
* Human vs Human mode
* Human vs CPU mode (random valid moves or optional AI version)
* Safe input validation using `fgets` + `strtol`
* Automatic gravity-based piece placement
* Win detection (horizontal, vertical, diagonal)
* Draw detection

## File Overview

* **connect_four.c** – Core game logic (board handling, move placement, win checking, game loop)
* **io_engine.c** – Input/output handling, move validation, optional CPU move generation
* **connect_four.h** – Shared constants and function prototypes
* **Makefile** – Build configuration

## Build & Run (Linux/macOS)

```bash
make
./c_nnect_four
```

## Build & Run (Windows with MinGW)

```bash
mingw32-make
c_nnect_four.exe
```

## How to Play

1. Start the program.
2. Choose Human vs Human or Human vs CPU.
3. Enter a column number (1–7).
4. The piece automatically drops to the lowest available row.
5. The game continues until one player connects four pieces or the board fills up.
