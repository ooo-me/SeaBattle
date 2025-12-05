# SeaBattle Architecture - Action Source Abstraction

## Overview

This document describes the refactored architecture for the SeaBattle game, which now supports both local and network gameplay through a unified action source abstraction.

## Key Components

### IActionSource Interface

The `IActionSource` interface is the core abstraction that defines how game actions are processed. It provides:

- **ShotCallback**: Notified when a shot action is taken
- **ResultCallback**: Notified when a shot result is determined
- **ErrorCallback**: Notified when errors occur
- **PlayerSwitchCallback**: Notified when player turn changes

All implementations must provide methods for:
- `processShot()`: Process a shot from a player
- `isValidShot()`: Check if a shot is valid
- `getCurrentPlayer()`: Get the current player
- `initialize()`: Initialize the action source

### LocalActionSource

Implementation of `IActionSource` for local (hotseat) gameplay:
- Processes shots synchronously through the GameModel
- Immediately notifies callbacks about results
- Handles player switching for local two-player mode

### NetworkActionSource

Stub implementation of `IActionSource` for future network gameplay:
- Placeholder for network communication
- Will handle sending/receiving shots over network
- Will synchronize game state between connected clients

### GameModelAdapter

The adapter that bridges the UI layer with the game model:
- Uses an `IActionSource` to process all game actions
- Provides callbacks for UI updates
- Abstracts the underlying action source (local vs network)
- Ensures all game events flow through the unified proxy

### ActionSourceFactory

Factory pattern for creating action sources:
- `SourceType::Local`: Creates LocalActionSource for hotseat gameplay
- `SourceType::Network`: Creates NetworkActionSource for online gameplay

## Data Flow

```
User Input (GameScreen/MainWindow)
    ↓
GameModelAdapter::processShot()
    ↓
IActionSource::processShot()
    ↓
    ├→ LocalActionSource: Direct model execution
    │       ↓
    │   GameModel::shoot()
    │       ↓
    │   Callbacks (Result, PlayerSwitch, etc.)
    │
    └→ NetworkActionSource: Network communication
            ↓
        Network Layer (future implementation)
            ↓
        Callbacks (Result, PlayerSwitch, etc.)
    ↓
UI Updates through callbacks
```

## Benefits

1. **Separation of Concerns**: Game logic is separated from action source
2. **Extensibility**: Easy to add new action sources (AI, replay, etc.)
3. **Testability**: Each component can be tested independently
4. **Network Ready**: Foundation for network gameplay is in place
5. **Unified Event Flow**: All game events go through consistent callbacks

## Future Enhancements

### Network Implementation
- Implement actual network communication in NetworkActionSource
- Add connection management and synchronization
- Handle network errors and disconnections

### Additional Action Sources
- AIActionSource: For computer opponents
- ReplayActionSource: For replaying recorded games
- TutorialActionSource: For guided gameplay

## Usage Example

```cpp
// Create local game
auto adapter = std::make_unique<GameModelAdapter>(
    SeaBattle::ActionSourceFactory::SourceType::Local
);

// Set up callbacks
adapter->setCellUpdateCallback([](int player, int row, int col, CellState state) {
    // Update UI
});

adapter->setPlayerSwitchCallback([](int newPlayer) {
    // Switch UI view
});

// Start game
adapter->startGame();

// Process shot
bool hit = adapter->processShot(row, col);
```

## Migration Notes

The refactoring maintains full backward compatibility with the existing local gameplay:
- All existing game mechanics work unchanged
- UI integration remains the same through GameModelAdapter
- No breaking changes to the public API

The changes are purely internal architectural improvements that prepare the codebase for network functionality.
