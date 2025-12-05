# Architecture Diagram

## Component Layers

```
┌─────────────────────────────────────────────────────────────────┐
│                         User Interface Layer                     │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────────┐   │
│  │ WelcomeScreen│  │ MainWindow   │  │   GameScreen        │   │
│  │   - Start   │  │  - Orchestr. │  │   - Battlefield     │   │
│  │   - Server  │  │  - Network   │  │   - Visualization   │   │
│  │   - Client  │  │  - Local     │  │                     │   │
│  └─────────────┘  └──────────────┘  └─────────────────────┘   │
│         │                 │                      │               │
│  ┌──────────────┐  ┌─────────────────┐                         │
│  │ServerSettings│  │ClientSettings   │                         │
│  │Dialog        │  │Dialog           │                         │
│  └──────────────┘  └─────────────────┘                         │
└─────────────────────────────────────────────────────────────────┘
                      │                │
                      ▼                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Qt Integration Layer                        │
│                   ┌───────────────────┐                         │
│                   │  NetworkAdapter   │                         │
│                   │  - Qt Signals     │                         │
│                   │  - Thread Mgmt    │                         │
│                   └───────────────────┘                         │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Network Transport Layer                       │
│                      (Qt Independent)                            │
│  ┌─────────────────┐         ┌────────────────┐                │
│  │ ServerSession   │         │ ClientSession  │                │
│  │  - Accept conn. │         │  - Connect     │                │
│  │  - Port config  │         │  - Host/Port   │                │
│  └─────────────────┘         └────────────────┘                │
│           │                           │                          │
│           └───────────┬───────────────┘                          │
│                       ▼                                          │
│              ┌─────────────────┐                                │
│              │ NetworkSession  │                                │
│              │  - boost::asio  │                                │
│              │  - Async I/O    │                                │
│              │  - Callbacks    │                                │
│              └─────────────────┘                                │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Protocol Layer                              │
│                    (Pure C++ / JSON)                             │
│                   ┌───────────────────┐                         │
│                   │    Protocol.h     │                         │
│                   │  - Message Types  │                         │
│                   │  - Serialization  │                         │
│                   │  - JSON format    │                         │
│                   └───────────────────┘                         │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Game Logic Layer                            │
│  ┌────────────────┐              ┌───────────────────┐         │
│  │ GameModel      │              │ NetworkGameModel  │         │
│  │ (Local games)  │              │ (Network games)   │         │
│  │ - Both players │              │ - Own field only  │         │
│  │ - Full state   │              │ - Simplified      │         │
│  └────────────────┘              └───────────────────┘         │
│          │                                  │                    │
│          └──────────────┬───────────────────┘                    │
│                         ▼                                        │
│              ┌──────────────────────┐                           │
│              │   GameModelAdapter   │                           │
│              │   - Callbacks        │                           │
│              │   - State queries    │                           │
│              └──────────────────────┘                           │
└─────────────────────────────────────────────────────────────────┘
```

## Message Flow

### Server Mode (Game Creation)

```
User                WelcomeScreen       MainWindow          NetworkAdapter      ServerSession
 |                       |                   |                    |                   |
 |-- Click "Создать" -->|                   |                    |                   |
 |                       |-- Show Dialog --->|                   |                    |
 |                       |                   |                    |                   |
 |<------- ServerSettingsDialog ------------>|                   |                    |
 |                       |                   |                    |                   |
 |-- Set Port & Start -->|                   |                    |                   |
 |                       |                   |-- startServer ---->|                   |
 |                       |                   |                    |-- Create & Accept->|
 |                       |                   |                    |                   |
 |                       |                   |                    |<--Client Connect--|
 |                       |                   |<-connectionEstab.--|                   |
 |                       |<-- Start Game ----|                    |                   |
 |<-- Game Screen -------|                   |                    |                   |
```

### Client Mode (Joining Game)

```
User                WelcomeScreen       MainWindow          NetworkAdapter      ClientSession
 |                       |                   |                    |                   |
 |-- Click "Присоед." ->|                   |                    |                   |
 |                       |-- Show Dialog --->|                   |                    |
 |                       |                   |                    |                   |
 |<------- ClientSettingsDialog ------------>|                   |                    |
 |                       |                   |                    |                   |
 |-- Set Host/Port ----->|                   |                    |                   |
 |                       |                   |-- connectToServer->|                   |
 |                       |                   |                    |-- Connect ------->|
 |                       |                   |                    |                   |
 |                       |                   |                    |<------ Connected--|
 |                       |                   |<-connectionEstab.--|                   |
 |                       |<-- Start Game ----|                    |                   |
 |<-- Game Screen -------|                   |                    |                   |
```

### Network Gameplay (Shot Exchange)

```
Player A           MainWindow A        NetworkAdapter A    Network     NetworkAdapter B    MainWindow B
 |                     |                      |               |                |                  |
 |-- Click Cell ----->|                      |               |                |                  |
 |                     |-- Create Shot Msg -->|               |                |                  |
 |                     |                      |-- Serialize ->|                |                  |
 |                     |                      |               |-- TCP/IP ----->|                  |
 |                     |                      |               |                |-- Deserialize -->|
 |                     |                      |               |                |                  |
 |                     |                      |               |                |-- Process Shot ->|
 |                     |                      |               |                |-- Check Hit ---->|
 |                     |                      |               |                |                  |
 |                     |                      |               |                |<-- Result Msg ---|
 |                     |                      |               |<-- TCP/IP -----|                  |
 |                     |                      |<- Deserialize-|                |                  |
 |                     |<-- Update UI --------|               |                |                  |
 |<-- Visual Update ---|                      |               |                |                  |
```

## Data Flow

### Local Game Mode
```
User Input → MainWindow → GameModelAdapter → GameModel → Callback → MainWindow → GameScreen → User
```

### Network Game Mode (Server)
```
User Input → MainWindow → NetworkAdapter → ServerSession → TCP → ClientSession
                ↓                                                          ↓
        NetworkGameModel                                           NetworkAdapter
                ↓                                                          ↓
           Own Field State                                            MainWindow
                                                                           ↓
                                                                      GameScreen
```

### Network Game Mode (Client)
```
User Input → MainWindow → NetworkAdapter → ClientSession → TCP → ServerSession
                ↓                                                          ↓
        NetworkGameModel                                           NetworkAdapter
                ↓                                                          ↓
           Own Field State                                            MainWindow
                                                                           ↓
                                                                      GameScreen
```

## Key Dependencies

```
┌─────────────────────┐
│   Application       │
│   Dependencies      │
├─────────────────────┤
│                     │
│  ┌──────────────┐  │
│  │   Qt6        │  │  ← GUI, Signals/Slots
│  │  - Widgets   │  │
│  │  - Core      │  │
│  └──────────────┘  │
│                     │
│  ┌──────────────┐  │
│  │  boost       │  │  ← Networking
│  │  - asio      │  │
│  └──────────────┘  │
│                     │
│  ┌──────────────┐  │
│  │nlohmann_json │  │  ← Serialization
│  └──────────────┘  │
│                     │
│  ┌──────────────┐  │
│  │   C++20      │  │  ← Language Features
│  └──────────────┘  │
│                     │
└─────────────────────┘
```

## State Management

### Local Game
```
┌────────────────────────────────────┐
│         GameModel                  │
│  ┌──────────────────────────────┐ │
│  │  Player 1 Field              │ │
│  │  - Ships                     │ │
│  │  - Cell States               │ │
│  └──────────────────────────────┘ │
│  ┌──────────────────────────────┐ │
│  │  Player 2 Field              │ │
│  │  - Ships                     │ │
│  │  - Cell States               │ │
│  └──────────────────────────────┘ │
│  - Current Player                  │
│  - Game State                      │
└────────────────────────────────────┘
```

### Network Game (Each Player)
```
┌────────────────────────────────────┐
│      NetworkGameModel              │
│  ┌──────────────────────────────┐ │
│  │  My Field                    │ │
│  │  - Ships                     │ │
│  │  - Cell States               │ │
│  └──────────────────────────────┘ │
│                                    │
│      +                             │
│                                    │
│  ┌──────────────────────────────┐ │
│  │  Enemy Field State           │ │
│  │  (in MainWindow)             │ │
│  │  - Known shots only          │ │
│  │  - Hit/Miss/Destroyed        │ │
│  └──────────────────────────────┘ │
└────────────────────────────────────┘
```

## Threading Model

```
┌──────────────────────────────────────────────────────────┐
│                    Main Thread                           │
│  - Qt Event Loop                                         │
│  - UI Updates                                            │
│  - Signal/Slot Processing                                │
│  - Game Logic (GameModel, NetworkGameModel)              │
└──────────────────────────────────────────────────────────┘
                          │
                          ▼
┌──────────────────────────────────────────────────────────┐
│                  Network Thread                          │
│  - boost::asio io_context.run()                          │
│  - Async socket operations                               │
│  - Message serialization/deserialization                 │
│  - Callbacks posted back to main thread via Qt signals   │
└──────────────────────────────────────────────────────────┘
```

## Security Boundaries

```
                    Internet/Network
                          │
                          ▼
┌─────────────────────────────────────────────────┐
│            Validation Layer                     │
│  ┌───────────────────────────────────────────┐ │
│  │  - Message size limit (64KB)              │ │
│  │  - Coordinate bounds checking             │ │
│  │  │  - Field state validation               │ │
│  └───────────────────────────────────────────┘ │
└─────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────┐
│            Application Logic                    │
│  - Trusted game state                           │
│  - User interface                               │
│  - Local game model                             │
└─────────────────────────────────────────────────┘
```
