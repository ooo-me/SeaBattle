#pragma once

#include "IModel.h"

class Client;

class RemoteModel : public SeaBattle::IModel
{
public:
    using CellUpdateCallback = std::function<void(int player, int row, int col, SeaBattle::CellState state)>;
    using PlayerSwitchCallback = std::function<void(int newPlayer)>;
    using GameOverCallback = std::function<void(int winner)>;

    RemoteModel();
    ~RemoteModel() override;

    void StartGame() override;
    bool ProcessShot(int row, int col) override;

    const std::vector<SeaBattle::Ship>& GetPlayerShips(int player) const override;
    int GetCurrentPlayer() const override;
    int GetLocalPlayer() const override;
    SeaBattle::GameState GetGameState() const override;

    void setCellUpdateCallback(CellUpdateCallback callback) { m_cellUpdateCallback = callback; }
    void setPlayerSwitchCallback(PlayerSwitchCallback callback) { m_playerSwitchCallback = callback; }
    void setGameOverCallback(GameOverCallback callback) { m_gameOverCallback = callback; }

private:
    std::unique_ptr<Client> m_client;

    CellUpdateCallback m_cellUpdateCallback;
    PlayerSwitchCallback m_playerSwitchCallback;
    GameOverCallback m_gameOverCallback;
};
