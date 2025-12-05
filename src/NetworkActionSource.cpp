#include "NetworkActionSource.h"

namespace SeaBattle
{
    NetworkActionSource::NetworkActionSource(GameModel* model)
        : m_model(model)
        , m_connected(false)
    {
    }

    void NetworkActionSource::setShotCallback(ShotCallback callback)
    {
        m_shotCallback = callback;
    }

    void NetworkActionSource::setResultCallback(ResultCallback callback)
    {
        m_resultCallback = callback;
    }

    void NetworkActionSource::setErrorCallback(ErrorCallback callback)
    {
        m_errorCallback = callback;
    }

    void NetworkActionSource::setPlayerSwitchCallback(PlayerSwitchCallback callback)
    {
        m_playerSwitchCallback = callback;
    }

    bool NetworkActionSource::processShot(int player, int row, int col)
    {
        // Stub implementation - to be implemented when network layer is ready
        if (!m_connected)
        {
            if (m_errorCallback)
            {
                m_errorCallback("Not connected to network");
            }
            return false;
        }

        // Future: Send shot to network and wait for response
        // For now, just return false
        if (m_errorCallback)
        {
            m_errorCallback("Network mode not yet implemented");
        }
        return false;
    }

    bool NetworkActionSource::isValidShot(int player, int row, int col) const
    {
        // Stub implementation
        if (!m_connected)
        {
            return false;
        }

        // Future: Check with network state
        return false;
    }

    int NetworkActionSource::getCurrentPlayer() const
    {
        // Stub implementation
        return 0;
    }

    void NetworkActionSource::initialize()
    {
        // Stub implementation - future: initialize network connection
        m_connected = false;
    }

    void NetworkActionSource::connect(const std::string& host, int port)
    {
        // Stub implementation - future: establish connection
        m_connected = false;
        if (m_errorCallback)
        {
            m_errorCallback("Network connection not yet implemented");
        }
    }

    void NetworkActionSource::disconnect()
    {
        // Stub implementation
        m_connected = false;
    }

    bool NetworkActionSource::isConnected() const
    {
        return m_connected;
    }

    void NetworkActionSource::handleIncomingShot(const ShotAction& action)
    {
        // Stub implementation - future: handle shot from network
        if (m_shotCallback)
        {
            m_shotCallback(getCurrentPlayer(), action);
        }
    }

    void NetworkActionSource::handleIncomingResult(const ShotResult& result)
    {
        // Stub implementation - future: handle result from network
        if (m_resultCallback)
        {
            m_resultCallback(getCurrentPlayer(), result);
        }
    }

} // namespace SeaBattle
