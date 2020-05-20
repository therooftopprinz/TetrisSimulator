# TetrisSimulator

Tetris game coordinator using tetris protocol.

Use cases:
* Multiplayer Tetris Game with customizable game modes.
* Bot Training

## Building

```sh
./prepare_external.sh
mkdir build
cd build
```

## Server

```sh
make server
```

## Default client

```sh
make client
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
