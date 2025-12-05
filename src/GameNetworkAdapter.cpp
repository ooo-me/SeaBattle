#include "GameNetworkAdapter.h"

namespace SeaBattle::Network
{
    GameNetworkAdapter::GameNetworkAdapter()
        : client_(std::make_shared<NetworkClient>())
    {
        // Set up network client callbacks
        client_->setConnectionStatusCallback(
            [this](ConnectionStatus status, const std::string& message) {
                onConnectionStatusChanged(status, message);
            });
        
        client_->setMessageReceivedCallback(
            [this](std::unique_ptr<Message> message) {
                onMessageReceived(std::move(message));
            });
        
        client_->setSendCompleteCallback(
            [this](bool success, const std::string& error) {
                onSendComplete(success, error);
            });
    }

    GameNetworkAdapter::~GameNetworkAdapter()
    {
        disconnect();
    }

    void GameNetworkAdapter::connect(const std::string& host, uint16_t port, 
                                     const std::string& playerName)
    {
        playerName_ = playerName;
        
        // Start network thread
        networkThread_ = std::thread([this]() {
            client_->run();
        });
        
        // Initiate connection
        client_->connectAsync(host, port);
    }

    void GameNetworkAdapter::disconnect()
    {
        if (client_)
        {
            client_->disconnect();
            client_->stop();
        }
        
        if (networkThread_.joinable())
        {
            networkThread_.join();
        }
    }

    void GameNetworkAdapter::sendShootAction(int row, int col)
    {
        if (!client_->isConnected())
        {
            if (errorCallback_)
            {
                errorCallback_("Cannot send shoot action: not connected");
            }
            return;
        }
        
        auto message = std::make_unique<ShootRequestMessage>(row, col);
        client_->sendMessage(std::move(message));
    }

    void GameNetworkAdapter::sendPing()
    {
        if (!client_->isConnected())
        {
            return;
        }
        
        auto message = std::make_unique<PingMessage>();
        client_->sendMessage(std::move(message));
    }

    ConnectionStatus GameNetworkAdapter::getConnectionStatus() const
    {
        return client_->getStatus();
    }

    bool GameNetworkAdapter::isConnected() const
    {
        return client_->isConnected();
    }

    std::string GameNetworkAdapter::getStatusMessage() const
    {
        return lastStatusMessage_;
    }

    void GameNetworkAdapter::setGameActionCallback(GameActionCallback callback)
    {
        gameActionCallback_ = std::move(callback);
    }

    void GameNetworkAdapter::setGameStateChangeCallback(GameStateChangeCallback callback)
    {
        gameStateCallback_ = std::move(callback);
    }

    void GameNetworkAdapter::setConnectionErrorCallback(ConnectionErrorCallback callback)
    {
        errorCallback_ = std::move(callback);
    }

    void GameNetworkAdapter::onConnectionStatusChanged(ConnectionStatus status, 
                                                       const std::string& message)
    {
        lastStatusMessage_ = message;
        
        // Handle specific status changes
        switch (status)
        {
            case ConnectionStatus::Connected:
                // Send connect message with player name
                if (!playerName_.empty())
                {
                    auto connectMsg = std::make_unique<ConnectMessage>(playerName_);
                    client_->sendMessage(std::move(connectMsg));
                }
                break;
                
            case ConnectionStatus::Error:
            case ConnectionStatus::Timeout:
                if (errorCallback_)
                {
                    errorCallback_(message);
                }
                break;
                
            case ConnectionStatus::Disconnected:
                // Notify game of disconnect
                if (errorCallback_)
                {
                    errorCallback_("Disconnected from server");
                }
                break;
                
            default:
                break;
        }
    }

    void GameNetworkAdapter::onMessageReceived(std::unique_ptr<Message> message)
    {
        if (!message)
        {
            return;
        }
        
        switch (message->getType())
        {
            case MessageType::ShootResponse:
            {
                auto* response = dynamic_cast<ShootResponseMessage*>(message.get());
                if (response)
                {
                    handleShootResponse(*response);
                }
                break;
            }
            
            case MessageType::Error:
            {
                auto* error = dynamic_cast<ErrorMessage*>(message.get());
                if (error)
                {
                    handleError(*error);
                }
                break;
            }
            
            case MessageType::GameStart:
                if (gameStateCallback_)
                {
                    gameStateCallback_(GameState::Playing);
                }
                break;
                
            case MessageType::GameOver:
                if (gameStateCallback_)
                {
                    gameStateCallback_(GameState::GameOver);
                }
                break;
                
            case MessageType::Pong:
                // Could track latency here
                break;
                
            default:
                // Unknown or unhandled message type
                break;
        }
    }

    void GameNetworkAdapter::onSendComplete(bool success, const std::string& error)
    {
        if (!success && errorCallback_)
        {
            errorCallback_("Failed to send message: " + error);
        }
    }

    void GameNetworkAdapter::handleShootResponse(const ShootResponseMessage& response)
    {
        // Note: In a complete implementation, we would track pending shots to match
        // responses with requests. For this prototype, we use sentinel values to
        // indicate that coordinate tracking is not yet implemented.
        constexpr int UNKNOWN_COORDINATE = -1;
        
        if (gameActionCallback_)
        {
            // TODO: Implement shot tracking to correlate responses with requests
            // For now, notify with unknown coordinates and hit result
            gameActionCallback_(UNKNOWN_COORDINATE, UNKNOWN_COORDINATE, response.isHit());
        }
    }

    void GameNetworkAdapter::handleError(const ErrorMessage& error)
    {
        if (errorCallback_)
        {
            errorCallback_("Server error: " + error.getErrorText());
        }
    }

} // namespace SeaBattle::Network
