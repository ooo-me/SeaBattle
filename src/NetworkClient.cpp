#include "NetworkClient.h"

#include <iostream>

namespace SeaBattle::Network
{
    NetworkClient::NetworkClient()
        : socket_(io_context_)
        , timeout_timer_(io_context_)
        , status_(ConnectionStatus::Disconnected)
        , sending_(false)
    {
        receiveHeaderBuffer_.resize(MessageHeader::SIZE);
    }

    NetworkClient::~NetworkClient()
    {
        disconnect();
    }

    void NetworkClient::connectAsync(const std::string& host, uint16_t port, 
                                     std::chrono::milliseconds timeout)
    {
        if (status_ != ConnectionStatus::Disconnected)
        {
            setStatus(ConnectionStatus::Error, "Already connected or connecting");
            return;
        }

        setStatus(ConnectionStatus::Connecting, "Connecting to " + host + ":" + std::to_string(port));

        try
        {
            // Resolve the host
            boost::asio::ip::tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            // Set up timeout
            timeout_timer_.expires_after(timeout);
            timeout_timer_.async_wait(
                [this](const boost::system::error_code& error) {
                    handleTimeout(error);
                });

            // Start async connect
            boost::asio::async_connect(
                socket_,
                endpoints,
                [this](const boost::system::error_code& error, 
                      const boost::asio::ip::tcp::endpoint&) {
                    handleConnect(error);
                });
        }
        catch (const std::exception& e)
        {
            setStatus(ConnectionStatus::Error, std::string("Connection failed: ") + e.what());
        }
    }

    void NetworkClient::disconnect()
    {
        if (status_ == ConnectionStatus::Disconnected)
        {
            return;
        }

        setStatus(ConnectionStatus::Disconnecting, "Disconnecting...");

        // Cancel any pending operations
        boost::system::error_code ec;
        timeout_timer_.cancel(ec);
        
        if (socket_.is_open())
        {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }

        setStatus(ConnectionStatus::Disconnected, "Disconnected");
    }

    void NetworkClient::sendMessage(std::unique_ptr<Message> message)
    {
        if (status_ != ConnectionStatus::Connected)
        {
            if (sendCallback_)
            {
                sendCallback_(false, "Not connected");
            }
            return;
        }

        try
        {
            auto serialized = message->serialize();
            
            {
                std::lock_guard<std::mutex> lock(sendMutex_);
                sendQueue_.push(std::move(serialized));
            }

            // Trigger send if not already sending
            io_context_.post([this]() {
                processSendQueue();
            });
        }
        catch (const std::exception& e)
        {
            if (sendCallback_)
            {
                sendCallback_(false, std::string("Serialization error: ") + e.what());
            }
        }
    }

    ConnectionStatus NetworkClient::getStatus() const
    {
        return status_;
    }

    bool NetworkClient::isConnected() const
    {
        return status_ == ConnectionStatus::Connected;
    }

    void NetworkClient::setConnectionStatusCallback(ConnectionStatusCallback callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        statusCallback_ = std::move(callback);
    }

    void NetworkClient::setMessageReceivedCallback(MessageReceivedCallback callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        messageCallback_ = std::move(callback);
    }

    void NetworkClient::setSendCompleteCallback(SendCompleteCallback callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        sendCallback_ = std::move(callback);
    }

    void NetworkClient::run()
    {
        io_context_.run();
    }

    void NetworkClient::stop()
    {
        io_context_.stop();
    }

    void NetworkClient::setStatus(ConnectionStatus status, const std::string& message)
    {
        {
            std::lock_guard<std::mutex> lock(statusMutex_);
            status_ = status;
            lastError_ = message;
        }

        // Invoke callback
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (statusCallback_)
        {
            statusCallback_(status, message);
        }
    }

    void NetworkClient::handleConnect(const boost::system::error_code& error)
    {
        // Cancel timeout timer
        boost::system::error_code ec;
        timeout_timer_.cancel(ec);

        if (error)
        {
            setStatus(ConnectionStatus::Error, "Connection failed: " + error.message());
            return;
        }

        setStatus(ConnectionStatus::Connected, "Connected successfully");
        
        // Start receiving messages
        startReceive();
    }

    void NetworkClient::handleTimeout(const boost::system::error_code& error)
    {
        // If timer was cancelled, ignore
        if (error == boost::asio::error::operation_aborted)
        {
            return;
        }

        // Timeout occurred
        if (status_ == ConnectionStatus::Connecting)
        {
            boost::system::error_code ec;
            socket_.close(ec);
            setStatus(ConnectionStatus::Timeout, "Connection timeout");
        }
    }

    void NetworkClient::startReceive()
    {
        if (status_ != ConnectionStatus::Connected)
        {
            return;
        }

        // Read message header
        boost::asio::async_read(
            socket_,
            boost::asio::buffer(receiveHeaderBuffer_),
            [this](const boost::system::error_code& error, size_t bytesTransferred) {
                handleReceiveHeader(error, bytesTransferred);
            });
    }

    void NetworkClient::handleReceiveHeader(const boost::system::error_code& error, 
                                            size_t bytesTransferred)
    {
        if (error)
        {
            if (error == boost::asio::error::eof || 
                error == boost::asio::error::connection_reset)
            {
                setStatus(ConnectionStatus::Disconnected, "Connection closed by peer");
            }
            else
            {
                setStatus(ConnectionStatus::Error, "Receive error: " + error.message());
            }
            return;
        }

        try
        {
            // Deserialize header
            auto header = MessageHeader::deserialize(receiveHeaderBuffer_);

            // Prepare payload buffer
            if (header.payloadSize > 0)
            {
                receivePayloadBuffer_.resize(header.payloadSize);

                // Read payload
                boost::asio::async_read(
                    socket_,
                    boost::asio::buffer(receivePayloadBuffer_),
                    [this](const boost::system::error_code& error, size_t bytesTransferred) {
                        handleReceivePayload(error, bytesTransferred);
                    });
            }
            else
            {
                // No payload, process message with empty payload
                handleReceivePayload(boost::system::error_code(), 0);
            }
        }
        catch (const std::exception& e)
        {
            setStatus(ConnectionStatus::Error, 
                     std::string("Message deserialization error: ") + e.what());
        }
    }

    void NetworkClient::handleReceivePayload(const boost::system::error_code& error, 
                                             size_t bytesTransferred)
    {
        if (error)
        {
            if (error == boost::asio::error::eof || 
                error == boost::asio::error::connection_reset)
            {
                setStatus(ConnectionStatus::Disconnected, "Connection closed by peer");
            }
            else
            {
                setStatus(ConnectionStatus::Error, "Receive error: " + error.message());
            }
            return;
        }

        try
        {
            // Reconstruct full message
            auto header = MessageHeader::deserialize(receiveHeaderBuffer_);
            auto message = createMessageFromType(header.type);
            
            if (message)
            {
                message->deserializePayload(receivePayloadBuffer_);

                // Invoke callback
                std::lock_guard<std::mutex> lock(callbackMutex_);
                if (messageCallback_)
                {
                    messageCallback_(std::move(message));
                }
            }

            // Continue receiving
            startReceive();
        }
        catch (const std::exception& e)
        {
            setStatus(ConnectionStatus::Error, 
                     std::string("Message processing error: ") + e.what());
        }
    }

    void NetworkClient::processSendQueue()
    {
        std::lock_guard<std::mutex> lock(sendMutex_);

        if (sending_ || sendQueue_.empty())
        {
            return;
        }

        if (status_ != ConnectionStatus::Connected)
        {
            return;
        }

        sending_ = true;
        auto& data = sendQueue_.front();

        boost::asio::async_write(
            socket_,
            boost::asio::buffer(data),
            [this](const boost::system::error_code& error, size_t bytesTransferred) {
                handleSend(error, bytesTransferred);
            });
    }

    void NetworkClient::handleSend(const boost::system::error_code& error, 
                                   size_t bytesTransferred)
    {
        {
            std::lock_guard<std::mutex> lock(sendMutex_);
            sendQueue_.pop();
            sending_ = false;
        }

        if (error)
        {
            if (sendCallback_)
            {
                sendCallback_(false, "Send error: " + error.message());
            }

            if (error == boost::asio::error::eof || 
                error == boost::asio::error::connection_reset)
            {
                setStatus(ConnectionStatus::Disconnected, "Connection closed by peer");
            }
            else
            {
                setStatus(ConnectionStatus::Error, "Send error: " + error.message());
            }
            return;
        }

        if (sendCallback_)
        {
            sendCallback_(true, "");
        }

        // Process next message in queue
        io_context_.post([this]() {
            processSendQueue();
        });
    }

    std::unique_ptr<Message> NetworkClient::createMessageFromType(MessageType type)
    {
        switch (type)
        {
            case MessageType::Connect:
                return std::make_unique<ConnectMessage>();
            case MessageType::ShootRequest:
                return std::make_unique<ShootRequestMessage>();
            case MessageType::ShootResponse:
                return std::make_unique<ShootResponseMessage>();
            case MessageType::Error:
                return std::make_unique<ErrorMessage>();
            case MessageType::Ping:
                return std::make_unique<PingMessage>();
            case MessageType::Pong:
                return std::make_unique<PongMessage>();
            default:
                // For messages without specific class, return nullptr
                // Caller should handle this case
                return nullptr;
        }
    }

} // namespace SeaBattle::Network
