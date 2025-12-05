#pragma once

#include "Protocol.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <queue>

namespace SeaBattle::Network
{
    // Connection status enumeration
    enum class ConnectionStatus
    {
        Disconnected,   // Not connected
        Connecting,     // Connection in progress
        Connected,      // Successfully connected
        Error,          // Connection error occurred
        Timeout,        // Connection or operation timed out
        Disconnecting   // Graceful disconnect in progress
    };

    // Convert status to string for debugging
    inline std::string connectionStatusToString(ConnectionStatus status)
    {
        switch (status)
        {
            case ConnectionStatus::Disconnected: return "Disconnected";
            case ConnectionStatus::Connecting: return "Connecting";
            case ConnectionStatus::Connected: return "Connected";
            case ConnectionStatus::Error: return "Error";
            case ConnectionStatus::Timeout: return "Timeout";
            case ConnectionStatus::Disconnecting: return "Disconnecting";
            default: return "Unknown";
        }
    }

    // Callbacks for client events
    using ConnectionStatusCallback = std::function<void(ConnectionStatus status, const std::string& message)>;
    using MessageReceivedCallback = std::function<void(std::unique_ptr<Message> message)>;
    using SendCompleteCallback = std::function<void(bool success, const std::string& error)>;

    // Network client class using boost::asio
    class NetworkClient : public std::enable_shared_from_this<NetworkClient>
    {
    public:
        NetworkClient();
        ~NetworkClient();

        // Connection management
        void connectAsync(const std::string& host, uint16_t port, 
                         std::chrono::milliseconds timeout = std::chrono::seconds(10));
        void disconnect();
        
        // Message sending
        void sendMessage(std::unique_ptr<Message> message);
        
        // Status query
        ConnectionStatus getStatus() const;
        bool isConnected() const;
        
        // Callback setters
        void setConnectionStatusCallback(ConnectionStatusCallback callback);
        void setMessageReceivedCallback(MessageReceivedCallback callback);
        void setSendCompleteCallback(SendCompleteCallback callback);
        
        // Run the io_context (should be called in a separate thread or main loop)
        void run();
        void stop();

    private:
        // IO context and socket
        boost::asio::io_context io_context_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::steady_timer timeout_timer_;
        
        // Connection state
        std::atomic<ConnectionStatus> status_;
        std::string lastError_;
        std::mutex statusMutex_;
        
        // Callbacks
        ConnectionStatusCallback statusCallback_;
        MessageReceivedCallback messageCallback_;
        SendCompleteCallback sendCallback_;
        std::mutex callbackMutex_;
        
        // Receive buffer
        std::vector<uint8_t> receiveHeaderBuffer_;
        std::vector<uint8_t> receivePayloadBuffer_;
        
        // Send queue
        std::queue<std::vector<uint8_t>> sendQueue_;
        std::mutex sendMutex_;
        bool sending_;
        
        // Internal methods
        void setStatus(ConnectionStatus status, const std::string& message = "");
        void handleConnect(const boost::system::error_code& error);
        void handleTimeout(const boost::system::error_code& error);
        void startReceive();
        void handleReceiveHeader(const boost::system::error_code& error, size_t bytesTransferred);
        void handleReceivePayload(const boost::system::error_code& error, size_t bytesTransferred);
        void processSendQueue();
        void handleSend(const boost::system::error_code& error, size_t bytesTransferred);
        std::unique_ptr<Message> createMessageFromType(MessageType type);
    };

} // namespace SeaBattle::Network
