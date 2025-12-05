# Network Multiplayer Implementation

## Overview
This document describes the implementation of client-server network multiplayer functionality for the SeaBattle game.

## Architecture

### Network Protocol Layer (`Protocol.h`)
- **Independence**: Pure C++ implementation with no Qt dependencies
- **Serialization**: Uses nlohmann::json for message serialization
- **Message Types**: 
  - `Shot`: Shot request from current player
  - `ShotResult`: Result of a shot (hit/miss/destroyed)
  - `GameStart`: Game started notification
  - `GameOver`: Game ended notification
  - `PlayerSwitch`: Turn switched to other player
  - `Connect/ConnectAccept/Disconnect`: Connection management
  - `Error`: Error notifications
  - `ChatMessage`: Placeholder for future chat functionality

### Network Session Layer
- **`NetworkSession.h`**: Base class for network communication
  - Uses boost::asio for async I/O
  - Message-based protocol with newline delimiters
  - Callback-based event handling
  - No Qt dependencies

- **`ServerSession.h`**: Server-side session management
  - Accepts one client connection
  - Port configuration
  - Automatic connection handling

- **`ClientSession.h`**: Client-side session
  - Connects to server by host and port
  - Connection status tracking

### Qt Integration Layer
- **`NetworkAdapter.h`**: Qt wrapper for network sessions
  - Bridges network callbacks to Qt signals
  - Manages boost::asio io_context in separate thread
  - Provides Qt-friendly interface

### Game Logic
- **`NetworkGameModel.h`**: Simplified game model for network play
  - Each player manages only their own field
  - Processes opponent shots
  - Tracks ship destruction
  - Validates shots

### UI Components
- **`ServerSettingsDialog.h`**: Server configuration dialog
  - Port selection
  - Connection status display
  - Error handling

- **`ClientSettingsDialog.h`**: Client connection dialog
  - Host and port input
  - Connection status display
  - Error handling

### Main Window Integration
- **Network Game Mode**: Separate from local game mode
- **Role Management**: Tracks server vs client role
- **Message Routing**: Handles all network protocol messages
- **State Tracking**: Maintains enemy field state for validation

## Game Flow

### Server Mode
1. User clicks "Создать игру" (Create Game)
2. ServerSettingsDialog opens
3. User configures port and clicks start
4. Server starts accepting connections
5. When client connects, game starts
6. Server takes first turn

### Client Mode
1. User clicks "Присоединиться" (Join Game)
2. ClientSettingsDialog opens
3. User enters host/port and clicks connect
4. Client connects to server
5. When connection established, game starts
6. Client waits for server's first move

### Network Gameplay
1. **Turn-based**: Server starts, players alternate on miss
2. **Shot Validation**: Each client validates their own shots
3. **Results**: Server processes opponent shots, sends results
4. **Win Condition**: Game ends when all ships destroyed
5. **Disconnection**: Handled gracefully with user notification

## Key Features

### Extensibility
- Protocol supports future chat messages
- Modular architecture allows adding new message types
- Separate network and game logic layers

### Independence from GUI
- Network layer can be used without Qt
- Suitable for future CLI or automated testing
- Clean separation of concerns

### Error Handling
- Connection timeouts
- Network errors
- Disconnection handling
- Invalid shot detection

## Dependencies

### Added to conanfile.txt
```
boost/1.86.0
nlohmann_json/3.11.3
```

### Compiler Requirements
- C++20
- Qt6 (Widgets, Core)
- Boost (header-only, asio)
- nlohmann_json

## Files Added

### Network Layer (Qt-independent)
- `src/Protocol.h`: Protocol definitions
- `src/NetworkSession.h`: Base session class
- `src/ServerSession.h`: Server session
- `src/ClientSession.h`: Client session

### Qt Integration
- `src/NetworkAdapter.h`: Qt wrapper
- `src/NetworkGameModel.h`: Network game logic
- `src/ServerSettingsDialog.h`: Server UI
- `src/ClientSettingsDialog.h`: Client UI

### Modified Files
- `src/WelcomeScreen.h/cpp`: Added network game buttons
- `src/MainWindow.h/cpp`: Network game integration
- `src/ModelAdapter.h`: Added isValidShot method
- `src/CMakeLists.txt`: Added new files and dependencies
- `CMakeLists.txt`: Added boost and json dependencies
- `conanfile.txt`: Added dependencies
- `src/pch.h`: Added required headers

## Testing Recommendations

### Manual Testing
1. **Local Game**: Verify existing functionality still works
2. **Server Mode**: 
   - Start server
   - Verify waiting state
   - Connect with client
   - Play full game
3. **Client Mode**:
   - Connect to server
   - Verify turn order
   - Test disconnection handling
4. **Network Gameplay**:
   - Test hits and misses
   - Verify ship destruction
   - Test game over conditions
   - Test premature disconnection

### Future Integration Tests
The architecture supports headless server/client for automated testing:
```cpp
// Example pseudo-code
boost::asio::io_context io;
ServerSession server(io, 12345);
ClientSession client(io);
client.connect("localhost", 12345);
// Automated game flow testing
```

## Known Limitations

### Current Implementation
- Single game session per application instance
- No reconnection support
- No spectator mode
- No game state persistence
- No chat functionality (architecture prepared)

### Platform Notes
- Windows-focused build system (CMakePresets.json)
- Should work on Linux/macOS with Qt6 and boost
- Tested architecture but not full build

## Future Enhancements

### High Priority
- Add comprehensive error handling
- Implement reconnection logic
- Add game state validation
- Improve ship destruction tracking

### Medium Priority
- Implement chat functionality
- Add connection quality indicators
- Support multiple concurrent games
- Add game replay functionality

### Low Priority
- Spectator mode
- Tournament mode
- Custom ship placement in network games
- Game statistics tracking
