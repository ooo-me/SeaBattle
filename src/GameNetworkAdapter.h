#pragma once

#include "NetworkClient.h"
#include "Model.h"

#include <memory>
#include <functional>
#include <string>
#include <thread>

namespace SeaBattle::Network
{
    // Game action callback types
    using GameActionCallback = std::function<void(int row, int col, bool hit)>;
    using GameStateChangeCallback = std::function<void(GameState state)>;
    using ConnectionErrorCallback = std::function<void(const std::string& error)>;

    /**
     * @brief Adapter class that bridges NetworkClient with game logic
     * 
     * This class provides a higher-level interface for game interactions
     * over the network, translating game actions to network messages
     * and vice versa. It operates without Qt dependencies and uses
     * standard C++20 and boost libraries.
     */
    class GameNetworkAdapter
    {
    public:
        GameNetworkAdapter();
        ~GameNetworkAdapter();

        // Connection management
        void connect(const std::string& host, uint16_t port, const std::string& playerName);
        void disconnect();
        
        // Game actions
        void sendShootAction(int row, int col);
        void sendPing();
        
        // Status queries
        ConnectionStatus getConnectionStatus() const;
        bool isConnected() const;
        std::string getStatusMessage() const;
        
        // Callback setters
        void setGameActionCallback(GameActionCallback callback);
        void setGameStateChangeCallback(GameStateChangeCallback callback);
        void setConnectionErrorCallback(ConnectionErrorCallback callback);

    private:
        std::shared_ptr<NetworkClient> client_;
        std::thread networkThread_;
        
        GameActionCallback gameActionCallback_;
        GameStateChangeCallback gameStateCallback_;
        ConnectionErrorCallback errorCallback_;
        
        std::string playerName_;
        std::string lastStatusMessage_;
        
        // Internal event handlers
        void onConnectionStatusChanged(ConnectionStatus status, const std::string& message);
        void onMessageReceived(std::unique_ptr<Message> message);
        void onSendComplete(bool success, const std::string& error);
        
        void handleShootResponse(const ShootResponseMessage& response);
        void handleError(const ErrorMessage& error);
    };

} // namespace SeaBattle::Network
