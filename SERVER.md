# SeaBattle Server Documentation

## Overview

The SeaBattle server is implemented using Boost.Asio/Beast and provides a game server that:
- Accepts one client connection at a time
- Manages game sessions with AI opponent
- Uses a text-based protocol for client-server communication
- Is completely isolated from Qt/GUI (pure C++20 with std/boost)

## Building

### Building with Qt GUI (default)

By default, the project builds both the GUI application and the server:

```bash
cmake -B build
cmake --build build
```

This requires Qt6 to be installed.

### Building Server Only (without Qt)

To build only the server without Qt dependencies:

```bash
cmake -B build -DBUILD_SERVER_ONLY=ON
cmake --build build
```

This creates the `SeaBattleServer` executable in the `build/src` directory and does not require Qt6.

## Running the Server

```bash
./SeaBattleServer [port]
```

Default port is 8080 if not specified.

Example:
```bash
./SeaBattleServer 8080
```

The server will start and wait for client connections.

## Protocol

The server uses a simple text-based protocol where each message is terminated by a newline (`\n`).

### Client to Server Messages

#### JOIN_GAME
Client requests to join the game.
```
JOIN_GAME <playerName>
```

#### READY
Client signals they are ready to start the game.
```
READY
```

#### SHOOT
Client shoots at specific coordinates.
```
SHOOT <row> <col>
```
- `row`: 0-9
- `col`: 0-9

#### QUIT
Client disconnects from the game.
```
QUIT
```

### Server to Client Messages

#### GAME_STARTED
Game has started, client is assigned a player number.
```
GAME_STARTED <playerNumber>
```
- `playerNumber`: 0 or 1 (currently always 0 for the single client)

#### YOUR_TURN
It's the client's turn to shoot.
```
YOUR_TURN
```

#### SHOOT_RESULT
Result of the client's shot.
```
SHOOT_RESULT <row> <col> <result>
```
- `result`: MISS, HIT, DESTROYED, or INVALID

#### OPPONENT_SHOT
The AI opponent shot at these coordinates.
```
OPPONENT_SHOT <row> <col> <result>
```
- `result`: MISS, HIT, DESTROYED, or INVALID

#### GAME_OVER
The game has ended.
```
GAME_OVER <winner>
```
- `winner`: Player number who won (0 = client, 1 = AI)

#### ERROR
An error occurred.
```
ERROR <errorMessage>
```

## Game Flow

1. Client connects to the server
2. Client sends `JOIN_GAME <name>`
3. Server responds with `GAME_STARTED 0`
4. Client sends `READY`
5. Server responds with `YOUR_TURN`
6. Client sends `SHOOT <row> <col>`
7. Server responds with `SHOOT_RESULT`
8. If client hit, server sends `YOUR_TURN` again
9. If client missed, AI takes turn and sends `OPPONENT_SHOT` messages
10. Steps 6-9 repeat until game ends
11. Server sends `GAME_OVER <winner>`

## Architecture

### Key Classes

- **GameServer**: Main server class that listens for connections
  - Uses `boost::asio::io_context` for async I/O
  - Accepts connections on a TCP port
  - Creates GameSession for each client

- **GameSession**: Manages a single client connection
  - Handles message parsing and serialization
  - Manages game state
  - Simulates AI opponent
  - Uses `GameModel` for game logic

- **Protocol**: Namespace containing message types and parser
  - All message classes inherit from `Message`
  - `MessageParser` for deserializing incoming messages
  - Each message has a `serialize()` method

- **GameModel**: Core game logic (from existing codebase)
  - Manages two game fields (player and opponent)
  - Handles ship placement
  - Processes shots
  - Determines game state

### Session States

- **WaitingForClient**: Initial state
- **ClientConnected**: Client connected, waiting for JOIN_GAME
- **GameInProgress**: Game is active
- **GameOver**: Game has ended
- **Closed**: Session is closed

## Example Client Interaction

```
Client: JOIN_GAME TestPlayer
Server: GAME_STARTED 0

Client: READY
Server: YOUR_TURN

Client: SHOOT 0 0
Server: SHOOT_RESULT 0 0 MISS
Server: OPPONENT_SHOT 3 6 MISS
Server: YOUR_TURN

Client: SHOOT 2 2
Server: SHOOT_RESULT 2 2 HIT
Server: YOUR_TURN

Client: SHOOT 2 3
Server: SHOOT_RESULT 2 3 HIT
Server: YOUR_TURN

... (game continues) ...

Server: GAME_OVER 0

Client: QUIT
```

## AI Opponent

The server includes a simple AI opponent that:
- Makes random valid shots
- Continues shooting while it hits
- Stops after a miss (switches turn to player)

The AI logic can be enhanced in the future for smarter gameplay.

## Thread Safety

The server uses asynchronous I/O with Boost.Asio:
- All I/O operations are asynchronous
- Callbacks execute in the io_context thread
- No additional threading required for basic operation

## Error Handling

- Invalid messages receive `ERROR` response
- Network errors close the session
- Invalid shots return `SHOOT_RESULT` with `INVALID` status
- Out-of-turn shots return `ERROR` response

## Future Enhancements

Possible improvements:
- Support for multiple simultaneous games (multiple client pairs)
- Reconnection support
- Game spectator mode
- Enhanced AI with difficulty levels
- Statistics and logging
- WebSocket support (Beast already provides this)
- TLS/SSL encryption
