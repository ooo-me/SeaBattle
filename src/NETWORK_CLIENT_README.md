# SeaBattle Network Client Documentation

## Overview

This document describes the network client implementation for SeaBattle using boost::asio/beast with comprehensive connection status handling.

## Architecture

The network client implementation consists of three main components:

### 1. Protocol Layer (`Protocol.h`)
Defines the message protocol for client-server communication:

- **Message Types**: Connect, Disconnect, GameStart, GameOver, ShootRequest, ShootResponse, Error, Ping/Pong
- **Message Classes**: Base `Message` class with concrete implementations for each message type
- **Serialization**: Binary protocol with message header (type + payload size) followed by payload
- **Result Codes**: Success, InvalidMove, NotYourTurn, GameNotStarted, ServerError

### 2. Network Client (`NetworkClient.h/cpp`)
Core networking layer using boost::asio:

- **Async Operations**: Non-blocking connect, send, and receive operations
- **Connection Status Tracking**: 
  - `Disconnected`: Initial state, no connection
  - `Connecting`: Connection attempt in progress
  - `Connected`: Successfully connected to server
  - `Error`: Connection error occurred
  - `Timeout`: Operation timed out
  - `Disconnecting`: Graceful disconnect in progress

- **Features**:
  - Automatic timeout handling for connections
  - Message queue for sending
  - Callback-based event system
  - Thread-safe status updates
  - Error handling and recovery

### 3. Game Network Adapter (`GameNetworkAdapter.h/cpp`)
High-level game interface:

- Bridges NetworkClient with game logic
- Translates game actions to network messages
- Manages network thread lifecycle
- Provides game-specific callbacks
- No Qt dependencies (C++20 + boost only)

## Usage Examples

### Basic Client Usage

```cpp
#include "NetworkClient.h"

using namespace SeaBattle::Network;

// Create client
auto client = std::make_shared<NetworkClient>();

// Set up callbacks
client->setConnectionStatusCallback(
    [](ConnectionStatus status, const std::string& message) {
        std::cout << "Status: " << connectionStatusToString(status) 
                  << " - " << message << std::endl;
    });

client->setMessageReceivedCallback(
    [](std::unique_ptr<Message> message) {
        // Handle received message
        switch (message->getType()) {
            case MessageType::ShootResponse:
                auto* response = dynamic_cast<ShootResponseMessage*>(message.get());
                // Process response...
                break;
        }
    });

// Start client thread
std::thread clientThread([client]() {
    client->run();
});

// Connect to server
client->connectAsync("localhost", 8080, std::chrono::seconds(10));

// Wait for connection...
std::this_thread::sleep_for(std::chrono::seconds(2));

// Send message
if (client->isConnected()) {
    auto shootMsg = std::make_unique<ShootRequestMessage>(5, 5);
    client->sendMessage(std::move(shootMsg));
}

// Cleanup
client->disconnect();
client->stop();
clientThread.join();
```

### Using Game Network Adapter

```cpp
#include "GameNetworkAdapter.h"

using namespace SeaBattle::Network;

// Create adapter
GameNetworkAdapter adapter;

// Set up game callbacks
adapter.setGameActionCallback(
    [](int row, int col, bool hit) {
        std::cout << "Shot at (" << row << "," << col << ") - "
                  << (hit ? "HIT!" : "MISS") << std::endl;
    });

adapter.setConnectionErrorCallback(
    [](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
    });

// Connect to server
adapter.connect("localhost", 8080, "PlayerName");

// Send game action
adapter.sendShootAction(3, 7);

// Check status
if (adapter.isConnected()) {
    std::cout << "Connected! Status: " << adapter.getStatusMessage() << std::endl;
}

// Cleanup
adapter.disconnect();
```

## Message Protocol Specification

### Message Format

All messages follow this binary format:

```
[Header: 5 bytes] [Payload: N bytes]

Header:
  - Byte 0: MessageType (uint8)
  - Bytes 1-4: Payload size (uint32, big-endian)

Payload:
  - Message-specific data
```

### Message Types

1. **Connect** (Type: 0)
   - Payload: Player name (UTF-8 string)
   
2. **ConnectAck** (Type: 1)
   - Payload: None
   
3. **Disconnect** (Type: 2)
   - Payload: None
   
4. **GameStart** (Type: 10)
   - Payload: Game configuration
   
5. **GameOver** (Type: 11)
   - Payload: Winner info
   
6. **ShootRequest** (Type: 20)
   - Payload: 2 bytes (row, col)
   
7. **ShootResponse** (Type: 21)
   - Payload: 2 bytes (result code, hit flag)
   
8. **Error** (Type: 30)
   - Payload: Error message (UTF-8 string)
   
9. **Ping** (Type: 40)
   - Payload: None
   
10. **Pong** (Type: 41)
    - Payload: None

## Connection Status Flow

```
Disconnected
    |
    | connectAsync()
    v
Connecting ----timeout----> Timeout
    |                           |
    | success                   | disconnect()
    v                           v
Connected ----error------> Error --> Disconnected
    |                       |
    | disconnect()          | disconnect()
    v                       v
Disconnecting ---------> Disconnected
```

## Thread Safety

- All public methods of `NetworkClient` are thread-safe
- Callbacks are invoked from the io_context thread
- Status updates use atomic operations
- Send queue is protected by mutex

## Error Handling

The client handles various error scenarios:

1. **Connection Timeout**: Configurable timeout for connection attempts
2. **Network Errors**: Boost error codes translated to connection status
3. **Disconnection Detection**: EOF and connection reset errors
4. **Message Errors**: Serialization/deserialization exceptions
5. **Send Errors**: Queue management and error callbacks

## Building

### Requirements
- C++20 compiler
- Boost 1.83 or higher (boost::asio, boost::system)
- CMake 3.15+

### Integration with Project

The network client is integrated into the main SeaBattle application:

```cmake
find_package(Boost REQUIRED COMPONENTS system)

target_sources(SeaBattle PRIVATE
    NetworkClient.cpp
    NetworkClient.h
    GameNetworkAdapter.cpp
    GameNetworkAdapter.h
    Protocol.h
)

target_link_libraries(SeaBattle PRIVATE
    Boost::system
)
```

## Testing

A standalone test program is provided in `ClientExample.cpp` that demonstrates:
- Connection management
- Status callbacks
- Message sending
- Error handling
- Timeout behavior

## Future Enhancements

Potential improvements for future iterations:

1. SSL/TLS support using boost::asio::ssl
2. Message compression
3. Automatic reconnection
4. Connection pooling
5. Latency tracking
6. Message acknowledgment system
7. Binary serialization using boost::serialization or protobuf
8. WebSocket support using boost::beast

## Notes

- The implementation is Qt-independent and uses standard C++20 + boost
- All network operations are asynchronous for responsive UI
- The protocol is extensible - new message types can be easily added
- Connection status changes trigger callbacks for UI updates
- The client maintains a send queue for reliable message delivery
