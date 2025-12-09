#pragma once

#include "Model.h"

#include <functional>

class GameModelAdapter : public SeaBattle::IModel
{
public:
    using CellUpdateCallback = std::function<void(int player, int row, int col, SeaBattle::CellState state)>;
    using GameStateCallback = std::function<void(SeaBattle::GameState state)>;
    using PlayerSwitchCallback = std::function<void(int newPlayer)>;
    using GameOverCallback = std::function<void(int winner)>;

    GameModelAdapter(SeaBattle::GameModel& sharedModel);

    void StartGame() override;
    bool ProcessShot(int row, int col) override;

    // Методы для получения состояния для GUI
    SeaBattle::GameState GetGameState() const override { return m_model.GetGameState(); }
    int GetCurrentPlayer() const override { return m_model.GetCurrentPlayer(); }
    const std::vector<SeaBattle::Ship>& GetPlayerShips(int player) const override;

    SeaBattle::CellState getPlayerCellState(int player, int row, int col) const;
    SeaBattle::CellState getEnemyCellState(int player, int row, int col) const;

    // Установка callback'ов
    void setCellUpdateCallback(CellUpdateCallback callback) { m_cellUpdateCallback = callback; }
    void setGameStateCallback(GameStateCallback callback) { m_gameStateCallback = callback; }
    void setPlayerSwitchCallback(PlayerSwitchCallback callback) { m_playerSwitchCallback = callback; }
    void setGameOverCallback(GameOverCallback callback) { m_gameOverCallback = callback; }

private:
    SeaBattle::GameModel& m_model;

    CellUpdateCallback m_cellUpdateCallback;
    GameStateCallback m_gameStateCallback;
    PlayerSwitchCallback m_playerSwitchCallback;
    GameOverCallback m_gameOverCallback;
};
