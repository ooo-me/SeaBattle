#pragma once

#include "Model.h"
#include "IActionSource.h"

#include <functional>

class GameModelAdapter
{
public:
    using CellUpdateCallback = std::function<void(int player, int row, int col, SeaBattle::CellState state)>;
    using GameStateCallback = std::function<void(SeaBattle::GameState state)>;
    using PlayerSwitchCallback = std::function<void(int newPlayer)>;
    using GameOverCallback = std::function<void(int winner)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    GameModelAdapter();
    explicit GameModelAdapter(SeaBattle::ActionSourceFactory::SourceType sourceType);

    void startGame();
    bool processShot(int row, int col);

    // Методы для получения состояния для GUI
    SeaBattle::CellState getPlayerCellState(int player, int row, int col) const;
    SeaBattle::CellState getEnemyCellState(int player, int row, int col) const;
    const std::vector<SeaBattle::Ship>& getPlayerShips(int player) const;
    int getCurrentPlayer() const { return model->getCurrentPlayer(); }
    SeaBattle::GameState getGameState() const { return model->getGameState(); }

    // Установка callback'ов
    void setCellUpdateCallback(CellUpdateCallback callback) { cellUpdateCallback = callback; }
    void setGameStateCallback(GameStateCallback callback) { gameStateCallback = callback; }
    void setPlayerSwitchCallback(PlayerSwitchCallback callback) { playerSwitchCallback = callback; }
    void setGameOverCallback(GameOverCallback callback) { gameOverCallback = callback; }
    void setErrorCallback(ErrorCallback callback) { errorCallback = callback; }

private:
    void initializeActionSource();

    std::unique_ptr<SeaBattle::GameModel> model;
    std::unique_ptr<SeaBattle::IActionSource> actionSource;
    SeaBattle::ActionSourceFactory::SourceType sourceType;

    CellUpdateCallback cellUpdateCallback;
    GameStateCallback gameStateCallback;
    PlayerSwitchCallback playerSwitchCallback;
    GameOverCallback gameOverCallback;
    ErrorCallback errorCallback;
};
