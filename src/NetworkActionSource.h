#pragma once

#include "IActionSource.h"
#include "Model.h"

namespace SeaBattle
{
    // Network action source for online gameplay (stub for future implementation)
    class NetworkActionSource : public IActionSource
    {
    public:
        explicit NetworkActionSource(GameModel* model);
        ~NetworkActionSource() override = default;

        // IActionSource interface
        void setShotCallback(ShotCallback callback) override;
        void setResultCallback(ResultCallback callback) override;
        void setErrorCallback(ErrorCallback callback) override;
        void setPlayerSwitchCallback(PlayerSwitchCallback callback) override;

        bool processShot(int player, int row, int col) override;
        bool isValidShot(int player, int row, int col) const override;
        int getCurrentPlayer() const override;
        void initialize() override;

        // Network-specific methods (for future implementation)
        void connect(const std::string& host, int port);
        void disconnect();
        bool isConnected() const;

    private:
        GameModel* m_model;
        ShotCallback m_shotCallback;
        ResultCallback m_resultCallback;
        ErrorCallback m_errorCallback;
        PlayerSwitchCallback m_playerSwitchCallback;
        bool m_connected;

        // Future: Network communication handlers
        void handleIncomingShot(const ShotAction& action);
        void handleIncomingResult(const ShotResult& result);
    };

} // namespace SeaBattle
