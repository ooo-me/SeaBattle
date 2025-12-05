#pragma once

#include "IActionSource.h"
#include "Model.h"

namespace SeaBattle
{
    // Local action source for hotseat gameplay
    class LocalActionSource : public IActionSource
    {
    public:
        explicit LocalActionSource(GameModel* model);
        ~LocalActionSource() override = default;

        // IActionSource interface
        void setShotCallback(ShotCallback callback) override;
        void setResultCallback(ResultCallback callback) override;
        void setErrorCallback(ErrorCallback callback) override;
        void setPlayerSwitchCallback(PlayerSwitchCallback callback) override;

        bool processShot(int player, int row, int col) override;
        bool isValidShot(int player, int row, int col) const override;
        int getCurrentPlayer() const override;
        void initialize() override;

    private:
        GameModel* m_model;
        ShotCallback m_shotCallback;
        ResultCallback m_resultCallback;
        ErrorCallback m_errorCallback;
        PlayerSwitchCallback m_playerSwitchCallback;
    };

} // namespace SeaBattle
