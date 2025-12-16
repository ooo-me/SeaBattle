#pragma once

#include "IModel.h"

class Client;

class RemoteModel : public SeaBattle::IModel
{
public:
    using CellUpdateCallback = std::function<void(int player, int row, int col, SeaBattle::CellState state)>;
    using PlayerSwitchCallback = std::function<void(int newPlayer)>;
    using GameOverCallback = std::function<void(bool)>;
    using StatusCallback = std::function<void(SeaBattle::ConnectionStatus status)>;
    using GameReadyCallback = std::function<void()>;
    using PlayerNamesCallback = std::function<void(const std::string& localName, const std::string& opponentName)>;

    RemoteModel();
    ~RemoteModel() override;

    void StartGame() override;
    bool ProcessShot(int row, int col) override;

    const std::vector<SeaBattle::Ship>& GetPlayerShips(int player) const override;
    int GetCurrentPlayer() const override;
    int GetLocalPlayer() const override;
    SeaBattle::GameState GetGameState() const override;

    // Player name support
    void SetPlayerName(const std::string& name) override;
    std::string GetLocalPlayerName() const override;
    std::string GetOpponentName() const override;

    void setCellUpdateCallback(CellUpdateCallback callback) { m_cellUpdateCallback = callback; }
    void setPlayerSwitchCallback(PlayerSwitchCallback callback) { m_playerSwitchCallback = callback; }
    void setGameOverCallback(GameOverCallback callback) { m_gameOverCallback = callback; }
    void setStatusCallback(StatusCallback callback) { m_statusCallback = callback; }
    void setGameReadyCallback(GameReadyCallback callback) { m_gameReadyCallback = callback; }
    void setPlayerNamesCallback(PlayerNamesCallback callback) { m_playerNamesCallback = callback; }

private:
    std::unique_ptr<Client> m_client;
    std::string m_playerName;

    CellUpdateCallback m_cellUpdateCallback;
    PlayerSwitchCallback m_playerSwitchCallback;
    GameOverCallback m_gameOverCallback;
    StatusCallback m_statusCallback;
    GameReadyCallback m_gameReadyCallback;
    PlayerNamesCallback m_playerNamesCallback;
};
