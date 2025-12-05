# SeaBattle Network Multiplayer

## Quick Start Guide

### What's New?
SeaBattle now supports network multiplayer! Play against friends over your local network.

### How to Play

#### Option 1: Create a Game (Server)
1. Launch SeaBattle
2. Click **"Создать игру"** (Create Game) button
3. Set a port number (default: 12345)
4. Click **"Создать игру"** (Create Game)
5. Wait for a friend to connect
6. You go first!

#### Option 2: Join a Game (Client)
1. Launch SeaBattle
2. Click **"Присоединиться"** (Join Game) button
3. Enter the server's IP address (e.g., 192.168.1.100)
4. Enter the port number (must match server's port)
5. Click **"Подключиться"** (Connect)
6. Wait for the server to make the first move

#### Option 3: Local Game (Unchanged)
1. Launch SeaBattle
2. Click **"Начать игру"** (Start Game)
3. Play hotseat mode as before

### Network Requirements

- **Local Network**: Both players must be on the same network
- **Firewall**: Server's firewall must allow incoming connections on the chosen port
- **IP Address**: Client needs to know server's IP address

### Finding Your IP Address

**Windows:**
```cmd
ipconfig
```
Look for "IPv4 Address" (e.g., 192.168.1.100)

**Linux/macOS:**
```bash
ifconfig
# or
ip addr show
```

### Gameplay Rules

#### Turn Order
- Server always goes first
- Players alternate after each miss
- If you hit, you get another turn
- If you miss, turn switches to opponent

#### Winning
- Destroy all opponent's ships to win
- Game over is automatically detected
- You can exit early from the game menu

#### Disconnection
- If connection is lost, you'll be notified
- Game will return to main menu
- No automatic reconnection (start a new game)

### Troubleshooting

#### "Failed to connect"
- Check IP address is correct
- Check port number matches
- Verify server is running and waiting
- Check firewall settings

#### "Failed to accept client"
- Check port is not already in use
- Verify firewall allows incoming connections
- Try a different port number

#### "Connection closed"
- Network connection was lost
- Opponent closed their game
- Start a new game

### Technical Details

For developers and technical users:

- **Protocol**: Custom JSON-based protocol over TCP
- **Library**: boost::asio for networking
- **Serialization**: nlohmann::json
- **Threading**: Separate thread for network I/O
- **Security**: Message size limited to 64KB

See documentation files for more details:
- `NETWORK_IMPLEMENTATION.md` - Technical implementation details
- `IMPLEMENTATION_SUMMARY.md` - Feature completion status
- `ARCHITECTURE_DIAGRAM.md` - Architecture and data flow diagrams

### Building from Source

#### Requirements
- C++20 compatible compiler
- Qt6 (Widgets, Core)
- CMake 3.31+
- Conan package manager
- boost 1.86.0 (header-only)
- nlohmann_json 3.11.3

#### Build Steps
```bash
# Install dependencies with Conan
conan install . --build=missing

# Configure with CMake
cmake --preset x64-release  # or appropriate preset

# Build
cmake --build --preset x64-release
```

### Known Limitations

- Only 1-on-1 games supported
- No reconnection after disconnect
- No spectator mode
- No chat (architecture ready, not implemented)
- Designed for trusted local networks only

### Future Enhancements

Potential future features:
- Chat functionality
- Game replay/history
- Multiple concurrent games
- Custom ship placement
- Tournament mode
- Reconnection support

### Contributing

Found a bug? Have a feature request? Please open an issue on GitHub!

### License

See LICENSE file for details.

---

**Enjoy your networked Sea Battle games! 🚢⚓**
