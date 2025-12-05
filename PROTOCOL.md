# SeaBattle Network Protocol Specification

## Overview

This document describes the network protocol for SeaBattle multiplayer mode. The protocol uses TCP for reliable communication over local networks in a 1-on-1 game mode. Messages are serialized using JSON format (nlohmann::json library).

## Design Principles

1. **Extensibility**: The protocol is designed to be easily extended with new message types
2. **Type Safety**: Strong typing through C++ interfaces and JSON schema validation
3. **Reliability**: TCP ensures ordered, reliable message delivery
4. **Simplicity**: JSON provides human-readable and debuggable messages
5. **Separation of Concerns**: Clear distinction between client and server responsibilities

## Message Format

All messages follow a standard JSON envelope format:

```json
{
  "type": "string",      // Message type identifier
  "version": "1.0",      // Protocol version
  "timestamp": 123456,   // Unix timestamp in milliseconds
  "payload": {}          // Message-specific data
}
```

### Base Message Fields

- **type** (required, string): Identifies the message type (e.g., "shot", "response", "error")
- **version** (required, string): Protocol version following semantic versioning (currently "1.0")
- **timestamp** (required, integer): Unix timestamp in milliseconds when the message was created
- **payload** (required, object): Message-specific data structure

## Core Message Types

### 1. Shot Message

Sent by the active player to take a shot at the opponent's field.

```json
{
  "type": "shot",
  "version": "1.0",
  "timestamp": 1701820000000,
  "payload": {
    "row": 5,           // Row coordinate (0-9)
    "col": 3            // Column coordinate (0-9)
  }
}
```

**Payload fields:**
- **row** (required, integer): Row coordinate, valid range [0, 9]
- **col** (required, integer): Column coordinate, valid range [0, 9]

### 2. Shot Response Message

Sent in response to a shot message, indicating the result.

```json
{
  "type": "shot_response",
  "version": "1.0",
  "timestamp": 1701820000100,
  "payload": {
    "row": 5,
    "col": 3,
    "result": "hit",      // "miss", "hit", or "destroyed"
    "game_over": false,   // true if this shot ended the game
    "winner": null        // player ID if game_over is true
  }
}
```

**Payload fields:**
- **row** (required, integer): Echo of the shot row coordinate
- **col** (required, integer): Echo of the shot column coordinate
- **result** (required, string): One of "miss", "hit", or "destroyed"
  - "miss": Shot missed all ships
  - "hit": Shot hit a ship but didn't destroy it
  - "destroyed": Shot destroyed a ship completely
- **game_over** (required, boolean): Indicates if the game has ended
- **winner** (optional, integer): Player ID (0 or 1) who won, only present if game_over is true

### 3. Error Message

Sent when an error occurs during message processing.

```json
{
  "type": "error",
  "version": "1.0",
  "timestamp": 1701820000200,
  "payload": {
    "code": "INVALID_SHOT",
    "message": "Shot coordinates out of bounds",
    "details": {
      "row": 15,
      "col": 3
    }
  }
}
```

**Payload fields:**
- **code** (required, string): Error code identifier
  - `INVALID_SHOT`: Shot coordinates are invalid
  - `INVALID_STATE`: Action not allowed in current game state
  - `PROTOCOL_ERROR`: Message format or protocol violation
  - `INTERNAL_ERROR`: Server-side error
  - `ALREADY_SHOT`: Cell was already shot at
  - `NOT_YOUR_TURN`: Player attempted to shoot out of turn
- **message** (required, string): Human-readable error description
- **details** (optional, object): Additional context-specific error information

### 4. Session Termination Message

Sent to gracefully terminate a game session.

```json
{
  "type": "session_end",
  "version": "1.0",
  "timestamp": 1701820000300,
  "payload": {
    "reason": "normal",
    "message": "Game completed successfully",
    "winner": 0
  }
}
```

**Payload fields:**
- **reason** (required, string): Termination reason
  - `normal`: Game completed normally
  - `forfeit`: One player forfeited
  - `disconnect`: Connection lost
  - `error`: Error forced termination
- **message** (optional, string): Additional information about termination
- **winner** (optional, integer): Winner player ID if applicable

### 5. Early Termination Message

Sent when a player wants to forfeit or abandon the game before completion.

```json
{
  "type": "forfeit",
  "version": "1.0",
  "timestamp": 1701820000400,
  "payload": {
    "player_id": 0,
    "reason": "Player requested game exit"
  }
}
```

**Payload fields:**
- **player_id** (required, integer): ID of the forfeiting player (0 or 1)
- **reason** (optional, string): Reason for forfeit

### 6. Chat Message (Reserved for Future Use)

Reserved message type for future chat functionality.

```json
{
  "type": "chat",
  "version": "1.0",
  "timestamp": 1701820000500,
  "payload": {
    "player_id": 0,
    "message": "Good game!"
  }
}
```

**Payload fields:**
- **player_id** (required, integer): ID of the message sender
- **message** (required, string): Chat message content (max 500 characters)

### 7. Game State Synchronization Message

Sent by server to synchronize game state with clients.

```json
{
  "type": "game_state",
  "version": "1.0",
  "timestamp": 1701820000600,
  "payload": {
    "current_player": 0,
    "player_id": 1,
    "game_status": "playing"
  }
}
```

**Payload fields:**
- **current_player** (required, integer): ID of player whose turn it is (0 or 1)
- **player_id** (required, integer): ID assigned to the receiving client
- **game_status** (required, string): Current game state ("waiting", "playing", "finished")

### 8. Connection Handshake Message

Sent during initial connection to establish session.

```json
{
  "type": "handshake",
  "version": "1.0",
  "timestamp": 1701820000700,
  "payload": {
    "client_version": "1.0.0",
    "player_name": "Player1"
  }
}
```

**Response:**
```json
{
  "type": "handshake_ack",
  "version": "1.0",
  "timestamp": 1701820000800,
  "payload": {
    "accepted": true,
    "player_id": 0,
    "session_id": "550e8400-e29b-41d4-a716-446655440000"
  }
}
```

## Protocol State Machine

### Client State Diagram

```
[Disconnected]
    |
    | (Connect)
    v
[Connecting] -----(Handshake)-----> [Authenticating]
    |                                      |
    | (Timeout/Error)                      | (Handshake ACK)
    v                                      v
[Disconnected]                      [WaitingForGame]
                                          |
                                          | (Game State: playing)
                                          v
                    +-------------> [MyTurn] <-------------+
                    |                  |                   |
                    |                  | (Send Shot)       |
                    |                  v                   |
                    |            [WaitingResponse]         |
    (Switch Turn)   |                  |                   | (Hit: continue turn)
                    |                  | (Shot Response)   |
                    |                  v                   |
                    +------------- [OpponentTurn] ---------+
                                          |
                                          | (Game Over)
                                          v
                                    [GameFinished]
                                          |
                                          | (Session End)
                                          v
                                    [Disconnected]
```

### Server State Diagram

```
[Idle]
    |
    | (Listen)
    v
[Listening]
    |
    | (Client Connect)
    v
[Authenticating] -----(Handshake OK)-----> [WaitingForPlayers]
    |                                              |
    | (Invalid Handshake)                          | (2 Players Connected)
    v                                              v
[Idle]                                       [GameActive]
                                                  |
                    +-----------------------------+
                    |                             |
                    | (Receive Shot)              |
                    v                             |
            [ProcessingShot] ---------(Valid)-----+
                    |                             |
                    | (Invalid)                   | (Game Over)
                    v                             v
            [SendError] --------------------------+
                                                  |
                                                  v
                                            [GameEnded]
                                                  |
                                                  | (Session End)
                                                  v
                                            [CleanUp]
                                                  |
                                                  v
                                            [Idle]
```

## Architecture Components

### Interface Hierarchy

```
IMessage (Abstract Base)
    ├── IMessageSerializer
    └── IMessageDeserializer

IMessageHandler (Abstract Base)
    ├── IShotHandler
    ├── IResponseHandler
    ├── IErrorHandler
    ├── ISessionHandler
    └── IChatHandler (future)

INetworkNode (Abstract Base)
    ├── IServer
    │   ├── ConnectionManager
    │   ├── SessionManager
    │   └── GameStateManager
    └── IClient
        ├── ConnectionHandler
        └── MessageQueue

IMessageRouter
    ├── Route messages to appropriate handlers
    └── Validate message types and versions
```

### Component Responsibilities

#### IMessage
- Base interface for all protocol messages
- Provides serialization/deserialization methods
- Ensures type safety and validation

#### IMessageHandler
- Processes specific message types
- Implements command pattern for extensibility
- Returns responses or errors

#### INetworkNode
- Abstracts network communication layer
- Manages TCP connections
- Handles message sending/receiving

#### IMessageRouter
- Routes incoming messages to appropriate handlers
- Validates protocol version compatibility
- Manages handler registration for extensibility

#### Server Components
- **ConnectionManager**: Manages client connections
- **SessionManager**: Maintains game sessions
- **GameStateManager**: Synchronizes game state between clients

#### Client Components
- **ConnectionHandler**: Maintains server connection
- **MessageQueue**: Queues outgoing messages for sending

## Extensibility Guidelines

### Adding New Message Types

1. Define the message structure in this specification
2. Create a new handler class implementing `IMessageHandler`
3. Register the handler with the message router
4. Update protocol version if breaking changes are introduced

Example:
```cpp
class NewFeatureHandler : public IMessageHandler {
public:
    std::string getMessageType() const override { return "new_feature"; }
    std::unique_ptr<IMessage> handle(const IMessage& message) override {
        // Process message and return response
    }
};

// Registration
router.registerHandler(std::make_unique<NewFeatureHandler>());
```

### Version Compatibility

- Minor version increments (1.0 -> 1.1): Backward compatible additions
- Major version increments (1.x -> 2.0): Breaking changes
- Servers should support multiple protocol versions when possible
- Clients should declare supported version in handshake

## Security Considerations

1. **Input Validation**: All message fields must be validated before processing
2. **Rate Limiting**: Implement message rate limits to prevent abuse
3. **Timeout Handling**: Connections should timeout after period of inactivity
4. **Buffer Limits**: Enforce maximum message size to prevent memory attacks
5. **Connection Limits**: Limit concurrent connections per IP address

## Performance Considerations

1. **Message Size**: Keep messages compact while maintaining readability
2. **Batching**: Consider batching multiple state updates when appropriate
3. **Compression**: For future versions, consider message compression
4. **Connection Pooling**: Reuse connections for multiple games when possible

## Error Handling

### Error Recovery Strategies

1. **Protocol Errors**: Log and close connection, inform client
2. **Invalid State**: Send error message, maintain connection
3. **Network Errors**: Implement reconnection with exponential backoff
4. **Timeout**: Send keep-alive messages to detect dead connections

### Client Error Handling

- Display user-friendly error messages
- Attempt reconnection on network failures
- Offer forfeit option on prolonged disconnection

### Server Error Handling

- Log all errors for debugging
- Gracefully handle client disconnections
- Clean up resources on session end

## Testing Strategy

1. **Unit Tests**: Test individual message handlers
2. **Integration Tests**: Test full message flow scenarios
3. **Load Tests**: Verify performance under multiple concurrent games
4. **Protocol Compliance**: Validate all messages against JSON schema
5. **Network Resilience**: Test behavior under poor network conditions

## Future Extensions

1. **Matchmaking**: Add lobby and matchmaking messages
2. **Spectator Mode**: Allow observers to watch games
3. **Replays**: Support game replay functionality
4. **Statistics**: Track and report player statistics
5. **Custom Rules**: Allow games with custom rule sets
6. **Tournaments**: Support tournament bracket management
7. **Friend System**: Add friend list and challenges
8. **Voice Chat**: Integrate voice communication
9. **Encryption**: Add TLS/SSL support for secure communication

## References

- JSON Format: [https://www.json.org/](https://www.json.org/)
- nlohmann/json Library: [https://github.com/nlohmann/json](https://github.com/nlohmann/json)
- TCP Protocol: RFC 793
- Semantic Versioning: [https://semver.org/](https://semver.org/)
