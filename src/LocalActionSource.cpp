#include "LocalActionSource.h"

namespace SeaBattle
{
    LocalActionSource::LocalActionSource(GameModel* model)
        : m_model(model)
    {
    }

    void LocalActionSource::setShotCallback(ShotCallback callback)
    {
        m_shotCallback = callback;
    }

    void LocalActionSource::setResultCallback(ResultCallback callback)
    {
        m_resultCallback = callback;
    }

    void LocalActionSource::setErrorCallback(ErrorCallback callback)
    {
        m_errorCallback = callback;
    }

    void LocalActionSource::setPlayerSwitchCallback(PlayerSwitchCallback callback)
    {
        m_playerSwitchCallback = callback;
    }

    bool LocalActionSource::processShot(int player, int row, int col)
    {
        if (!m_model)
        {
            if (m_errorCallback)
            {
                m_errorCallback("Model not initialized");
            }
            return false;
        }

        if (player != m_model->getCurrentPlayer())
        {
            if (m_errorCallback)
            {
                m_errorCallback("Not this player's turn");
            }
            return false;
        }

        if (!m_model->isValidShot(row, col))
        {
            if (m_errorCallback)
            {
                m_errorCallback("Invalid shot position");
            }
            return false;
        }

        // Notify about the shot action
        if (m_shotCallback)
        {
            ShotAction action{ row, col };
            m_shotCallback(player, action);
        }

        // Execute the shot
        bool hit = m_model->shoot(row, col);

        // Get the result state
        CellState resultState = m_model->getEnemyViewCellState(player, row, col);

        // Prepare result
        ShotResult result;
        result.hit = hit;
        result.resultState = resultState;
        result.gameOver = (m_model->getGameState() == GameState::GameOver);
        result.winner = result.gameOver ? m_model->getWinner() : -1;

        // Notify about the result
        if (m_resultCallback)
        {
            m_resultCallback(player, result);
        }

        // Notify about player switch if it happened
        if (!hit && !result.gameOver)
        {
            if (m_playerSwitchCallback)
            {
                m_playerSwitchCallback(m_model->getCurrentPlayer());
            }
        }

        return hit;
    }

    bool LocalActionSource::isValidShot(int player, int row, int col) const
    {
        if (!m_model)
        {
            return false;
        }

        if (player != m_model->getCurrentPlayer())
        {
            return false;
        }

        return m_model->isValidShot(row, col);
    }

    int LocalActionSource::getCurrentPlayer() const
    {
        return m_model ? m_model->getCurrentPlayer() : 0;
    }

    void LocalActionSource::initialize()
    {
        // For local action source, initialization is minimal
        // The model is already initialized externally
    }

} // namespace SeaBattle
