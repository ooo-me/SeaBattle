# SeaBattle Network Architecture

## Overview

This document describes the architectural design for the SeaBattle network multiplayer mode. The architecture follows SOLID principles and emphasizes extensibility, maintainability, and separation of concerns.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  ┌──────────────┐                        ┌──────────────┐   │
│  │  UI (Qt)     │                        │  Game Logic  │   │
│  │  - Screens   │◄──────────────────────►│  - Model     │   │
│  │  - Controls  │                        │  - Rules     │   │
│  └──────────────┘                        └──────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ Events & Commands
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Network Layer                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Message Router                          │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐          │   │
│  │  │  Shot    │  │ Response │  │  Error   │  ...     │   │
│  │  │ Handler  │  │ Handler  │  │ Handler  │          │   │
│  │  └──────────┘  └──────────┘  └──────────┘          │   │
│  └──────────────────────────────────────────────────────┘   │
│                            │                                 │
│  ┌─────────────────────────┼─────────────────────────────┐  │
│  │         Client Side     │         Server Side         │  │
│  │  ┌─────────────────┐    │    ┌─────────────────┐     │  │
│  │  │     Client      │    │    │     Server      │     │  │
│  │  │  - Connection   │    │    │  - Listener     │     │  │
│  │  │  - Send/Recv    │    │    │  - Sessions     │     │  │
│  │  │  - State Mgmt   │    │    │  - Broadcast    │     │  │
│  │  └─────────────────┘    │    └─────────────────┘     │  │
│  └─────────────────────────┴─────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ TCP/IP
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Transport Layer                           │
│         (Qt Network / Standard C++ Sockets)                  │
└─────────────────────────────────────────────────────────────┘
```

## Component Architecture

### 1. Message Layer

The message layer defines the protocol data structures and serialization.

#### Class Hierarchy

```
IMessage (Interface)
    │
    ├── BaseMessage (Abstract)
    │       │
    │       ├── ShotMessage
    │       ├── ShotResponseMessage
    │       ├── ErrorMessage
    │       ├── SessionEndMessage
    │       ├── ForfeitMessage
    │       ├── ChatMessage
    │       ├── GameStateMessage
    │       ├── HandshakeMessage
    │       └── HandshakeAckMessage
    │
    └── MessageFactory (Static)
```

**Responsibilities:**
- **IMessage**: Defines interface for all messages (getType, getVersion, toJson, validate)
- **BaseMessage**: Implements common message envelope (type, version, timestamp)
- **Specific Messages**: Implement payload-specific logic
- **MessageFactory**: Creates message instances from JSON

### 2. Handler Layer

The handler layer implements the Command pattern for processing messages.

#### Class Hierarchy

```
IMessageHandler (Interface)
    │
    ├── IShotHandler
    ├── IResponseHandler
    ├── IErrorHandler
    ├── ISessionHandler
    │   ├── ISessionEndHandler
    │   └── IForfeitHandler
    ├── IChatHandler
    ├── IGameStateHandler
    └── IHandshakeHandler
```

**Responsibilities:**
- **IMessageHandler**: Base interface for all handlers
- **Specific Handlers**: Process their respective message types
- Each handler can return a response message or nullptr

**Handler Registration:**
```cpp
// Example usage
auto router = std::make_unique<MessageRouter>();
router->registerHandler(std::make_unique<ShotHandlerImpl>());
router->registerHandler(std::make_unique<ErrorHandlerImpl>());
// ... register other handlers
```

### 3. Router Layer

The router layer dispatches messages to appropriate handlers.

#### Components

```
IMessageRouter (Interface)
    │
    └── MessageRouter (Implementation)
```

**Responsibilities:**
- Register/unregister message handlers
- Route incoming messages to appropriate handlers
- Validate protocol versions
- Validate message structure

**Key Methods:**
- `registerHandler(handler)`: Add a new handler
- `route(message)`: Process a message and return response
- `isVersionSupported(version)`: Check version compatibility

### 4. Network Layer

The network layer handles TCP communication.

#### Class Hierarchy

```
INetworkNode (Base Interface)
    │
    ├── IClient
    │   └── Client Implementation (Qt/std)
    │       ├── ConnectionHandler
    │       └── MessageQueue
    │
    └── IServer
        └── Server Implementation (Qt/std)
            ├── ConnectionManager
            ├── SessionManager
            └── GameStateManager
```

**Responsibilities:**

**INetworkNode:**
- Send/receive messages
- Connection state management
- Callbacks for events (message received, error, state change)

**IClient:**
- Connect to server
- Maintain single connection
- Queue outgoing messages

**IServer:**
- Listen for connections
- Manage multiple clients
- Broadcast messages
- Per-client message routing

**Supporting Classes:**

**IConnectionManager:**
- Track active client connections
- Add/remove clients
- Query connection state

**ISessionManager:**
- Create/close game sessions
- Map players to sessions
- Track session state

**IMessageQueue:**
- Buffer outgoing messages
- Ensure ordered delivery
- Handle backpressure

## Message Flow Sequences

### 1. Connection Establishment

```
Client                          Server
  |                               |
  |-- TCP Connect --------------->|
  |                               |
  |-- Handshake ----------------->|
  |    {client_version,          |
  |     player_name}             |
  |                              |
  |                              |-- Validate
  |                              |-- Assign Player ID
  |                              |-- Generate Session ID
  |                              |
  |<-- HandshakeAck -------------|
  |    {accepted: true,          |
  |     player_id: 0,            |
  |     session_id: "..."}       |
  |                              |
  |<-- GameState ----------------|
  |    {current_player: 0,       |
  |     player_id: 0,            |
  |     game_status: "waiting"}  |
  |                              |
```

### 2. Game Turn Flow

```
Player 1 (Client)                Server                Player 2 (Client)
      |                             |                          |
      |-- Shot ---------------------->|                        |
      |    {row: 5, col: 3}          |                        |
      |                              |                        |
      |                              |-- Process Shot         |
      |                              |-- Update Game State    |
      |                              |                        |
      |<-- ShotResponse --------------|                        |
      |    {row: 5, col: 3,          |                        |
      |     result: "hit"}           |                        |
      |                              |                        |
      |                              |-- ShotResponse -------->|
      |                              |    {row: 5, col: 3,    |
      |                              |     result: "hit"}     |
      |                              |                        |
      |                              |-- GameState ---------->|
      |                              |    {current_player: 0} |
```

### 3. Error Handling

```
Client                          Server
  |                               |
  |-- Shot ---------------------->|
  |    {row: 15, col: 3}         |
  |                              |
  |                              |-- Validate (FAIL)
  |                              |
  |<-- Error --------------------|
  |    {code: "INVALID_SHOT",    |
  |     message: "Out of bounds",|
  |     details: {row: 15, ...}} |
  |                              |
```

### 4. Game Termination

```
Player 1 (Client)                Server                Player 2 (Client)
      |                             |                          |
      |-- Shot ---------------------->|                        |
      |    {row: 9, col: 9}          |                        |
      |                              |                        |
      |                              |-- Process (Last Ship)  |
      |                              |                        |
      |<-- ShotResponse --------------|                        |
      |    {result: "destroyed",     |                        |
      |     game_over: true,         |                        |
      |     winner: 0}               |                        |
      |                              |                        |
      |                              |-- ShotResponse -------->|
      |                              |    {game_over: true,   |
      |                              |     winner: 0}         |
      |                              |                        |
      |-- SessionEnd --------------->|                        |
      |    {reason: "normal",        |                        |
      |     winner: 0}               |                        |
      |                              |                        |
      |                              |-- SessionEnd --------->|
      |                              |    {reason: "normal",  |
      |                              |     winner: 0}         |
      |                              |                        |
      |-- TCP Close ----------------->|                        |
      |                              |-- TCP Close ---------->|
```

## State Management

### Client States

```
┌─────────────┐
│ Disconnected│
└──────┬──────┘
       │
       │ connect()
       ▼
┌─────────────┐
│ Connecting  │
└──────┬──────┘
       │
       │ handshake_ack
       ▼
┌──────────────┐
│Authenticating│
└──────┬───────┘
       │
       │ game_state
       ▼
┌──────────────┐       game_state        ┌────────────┐
│WaitingForGame├───────(playing)────────►│   MyTurn   │
└──────────────┘                         └─────┬──────┘
                                               │
                                               │ shot
                                               ▼
                                         ┌─────────────┐
                                         │  Waiting    │
                                         │  Response   │
                                         └──────┬──────┘
                                                │
                                                │ shot_response
                                                ▼
       ┌────────────────────────────────────────┴───────┐
       │                                                │
       │ (miss)                                    (hit)│
       ▼                                                ▼
┌──────────────┐                              ┌────────────┐
│ OpponentTurn │                              │   MyTurn   │
└──────┬───────┘                              └────────────┘
       │
       │ game_state
       │ (my turn)
       ▼
┌────────────┐
│   MyTurn   │
└────────────┘
```

### Server States

```
┌──────┐
│ Idle │
└───┬──┘
    │
    │ listen()
    ▼
┌───────────┐
│ Listening │
└─────┬─────┘
      │
      │ client_connect
      ▼
┌─────────────────┐
│ Authenticating  │
│   (1 player)    │
└────────┬────────┘
         │
         │ handshake_ack
         ▼
┌─────────────────┐
│WaitingForPlayers│
│   (1 player)    │
└────────┬────────┘
         │
         │ client_connect
         ▼
┌─────────────────┐
│ Authenticating  │
│   (2 players)   │
└────────┬────────┘
         │
         │ handshake_ack
         ▼
┌─────────────┐           shot            ┌─────────────────┐
│ GameActive  ├────────received───────────►│ProcessingShot  │
│(both ready) │                            └────────┬────────┘
└─────┬───────┘                                     │
      │                                             │
      │                                   ┌─────────┴─────────┐
      │                                   │                   │
      │                              (valid)              (invalid)
      │                                   │                   │
      │                                   ▼                   ▼
      │                            ┌────────────┐      ┌──────────┐
      │                            │Send Response│      │SendError│
      │◄───────────────────────────┤& Update     │      └────┬─────┘
      │                            └─────┬──────┘            │
      │                                  │                   │
      │◄─────────────────────────────────┘───────────────────┘
      │
      │ game_over
      ▼
┌──────────┐
│GameEnded │
└────┬─────┘
     │
     │ session_end
     ▼
┌─────────┐
│ CleanUp │
└────┬────┘
     │
     ▼
┌─────┐
│ Idle│
└─────┘
```

## Extensibility Strategy

### Adding New Message Types

1. **Define Message Class**
   ```cpp
   class NewFeatureMessage : public BaseMessage
   {
   public:
       NewFeatureMessage(/* params */)
           : BaseMessage("new_feature")
       { }
       
       // Implement required methods
   protected:
       nlohmann::json getPayload() const override { /* ... */ }
       bool validatePayload() const override { /* ... */ }
   };
   ```

2. **Create Handler**
   ```cpp
   class INewFeatureHandler : public IMessageHandler
   {
   public:
       std::string getMessageType() const override 
       { 
           return "new_feature"; 
       }
       
       std::unique_ptr<IMessage> handle(const IMessage& msg) override
       {
           // Process message
           // Return response or nullptr
       }
   };
   ```

3. **Register Handler**
   ```cpp
   router->registerHandler(std::make_unique<NewFeatureHandlerImpl>());
   ```

4. **Update MessageFactory**
   ```cpp
   // Add case in createFromJson
   if (type == "new_feature")
       return NewFeatureMessage::fromJson(json);
   ```

### Version Management

**Minor Version Updates (Backward Compatible):**
- Add optional fields to existing messages
- Add new message types
- Increment minor version (1.0 → 1.1)
- Older clients continue to work

**Major Version Updates (Breaking Changes):**
- Modify existing message structure
- Change required fields
- Increment major version (1.x → 2.0)
- Implement version negotiation in handshake

**Version Support Strategy:**
```cpp
bool MessageRouter::isVersionSupported(const std::string& version) const
{
    // Support current and previous major version
    return version == "1.0" || version == "2.0";
}
```

## Threading Considerations

### Thread Safety Requirements

1. **Message Queue**: Must be thread-safe for concurrent access
2. **Connection Manager**: Protect client list with mutex
3. **Session Manager**: Thread-safe session operations
4. **Callbacks**: Execute callbacks on main thread (Qt event loop)

### Recommended Threading Model

**Client Side:**
- UI Thread: Qt event loop, user interactions
- Network Thread: Socket I/O, message sending/receiving

**Server Side:**
- Main Thread: Accept connections, coordinate
- Worker Threads: One per client connection
- Game Logic Thread: Process shots, manage state

### Qt Integration

Use Qt's signal/slot mechanism for thread-safe communication:
```cpp
class NetworkClient : public QObject
{
    Q_OBJECT
    
signals:
    void messageReceived(std::unique_ptr<IMessage> message);
    void errorOccurred(QString error);
    void stateChanged(ConnectionState state);
    
public slots:
    void sendMessage(const IMessage& message);
    void connectToServer(QString host, int port);
};
```

## Error Handling Strategy

### Error Categories

1. **Protocol Errors**: Invalid message format, unsupported version
   - Action: Send error message, log, may close connection
   
2. **Game Logic Errors**: Invalid moves, wrong turn
   - Action: Send error message, maintain connection
   
3. **Network Errors**: Connection lost, timeout
   - Action: Attempt reconnection, notify user
   
4. **Internal Errors**: Memory allocation, unexpected state
   - Action: Log, send error message, graceful shutdown

### Error Recovery

**Client:**
- Reconnection with exponential backoff
- Queue messages during reconnection
- UI feedback for connection issues

**Server:**
- Graceful client disconnection handling
- Session cleanup on errors
- Resource cleanup (sockets, memory)

## Testing Strategy

### Unit Tests

1. **Message Tests**
   - Serialization/deserialization
   - Validation logic
   - Edge cases (boundary values)

2. **Handler Tests**
   - Message processing logic
   - Response generation
   - Error conditions

3. **Router Tests**
   - Handler registration
   - Message routing
   - Version validation

### Integration Tests

1. **Client-Server Communication**
   - Full message flow
   - State synchronization
   - Error propagation

2. **Game Flow Tests**
   - Complete game scenarios
   - Turn management
   - Win conditions

### Network Tests

1. **Connection Tests**
   - Connect/disconnect
   - Reconnection
   - Multiple clients

2. **Reliability Tests**
   - Message ordering
   - No message loss
   - Handling network delays

3. **Load Tests**
   - Multiple concurrent games
   - Message throughput
   - Resource usage

## Security Considerations

### Input Validation

- Validate all message fields
- Check coordinate bounds
- Limit message size (prevent DOS)
- Rate limiting per client

### Connection Security

- Consider TLS/SSL for future versions
- Implement connection timeouts
- Limit concurrent connections per IP
- Validate handshake data

### Game State Security

- Server is authoritative
- Client cannot cheat by modifying state
- Validate game logic server-side
- Prevent replay attacks (timestamp validation)

## Performance Optimization

### Message Optimization

- Keep messages compact
- Binary protocol consideration for future
- Message batching for multiple updates
- Compression for large payloads

### Network Optimization

- Connection pooling
- Keep-alive messages
- TCP_NODELAY for low latency
- Buffer size tuning

### Memory Optimization

- Object pooling for frequent allocations
- Smart pointer usage
- Limit message queue size
- Clean up inactive sessions

## Future Enhancements

1. **Matchmaking System**
   - Lobby management
   - Player ranking
   - Friend system

2. **Replay System**
   - Record game messages
   - Playback functionality
   - Share replays

3. **Spectator Mode**
   - Read-only connections
   - Broadcast to multiple spectators
   - Delayed stream option

4. **Advanced Features**
   - Voice chat integration
   - Custom game rules
   - Tournament support
   - Statistics tracking

5. **Platform Expansion**
   - Internet play (not just LAN)
   - NAT traversal (UPnP, STUN/TURN)
   - Cross-platform compatibility
   - Mobile clients

## Conclusion

This architecture provides a solid foundation for network multiplayer functionality while maintaining extensibility for future enhancements. The clear separation of concerns and use of interfaces allows for easy testing, modification, and extension of the system.

The design follows industry best practices:
- **SOLID principles**: Single responsibility, open/closed, Liskov substitution, interface segregation, dependency inversion
- **Design patterns**: Command, Factory, Observer, Strategy
- **Clean architecture**: Layer separation, dependency inversion
- **Extensibility**: Plugin-style handler registration, version management
