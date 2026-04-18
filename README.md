# TetrisSimulator

Tetris game coordinator using tetris protocol.

Use cases:
* Multiplayer Tetris Game with customizable game modes.
* Bot Training

## Building

Configure once from the repository root (CMake fetches [Logless](https://github.com/therooftopprinz/Logless) automatically if the `Logless/` submodule is not present; otherwise the submodule tree is used):

```sh
cmake -S . -B build
cmake --build build
```

With a normal git checkout, CMake runs `git submodule update --init --recursive` once at configure time (disable with `-DTETRIS_INIT_GIT_SUBMODULES=OFF`). If any of BFC, cum, or Logless are still missing, they are cloned with `FetchContent` into the build tree or under `TETRIS_BFC_ROOT` / `TETRIS_CUM_ROOT` / Logless paths. Override repository URLs or tags with cache variables `TETRIS_BFC_GIT_REPOSITORY`, `TETRIS_CUM_GIT_REPOSITORY`, `TETRIS_LOGLESS_GIT_REPOSITORY`, and the corresponding `*_GIT_TAG` variables if needed.

## Server

```sh
cmake --build build --target server
```

## Default client

```sh
cmake --build build --target client
```

Program input arguments:
* --server=IP:PORT - the transport address of the server
* --cmd=COMMAND - command that will run upon startup

### Command Line Interface
* create - create a game, the client will serve as a game master
  * lock - lockdown timeout setting in milliseconds
  * target - target timeout setting in milliseconds
* join - join existing game
  * id - game id to join

## Standard call flow:

![Call Flow](https://github.com/therooftopprinz/TetrisSimulator/raw/master/interface/protocol.png)
