#pragma once

#include "Protocol.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <queue>
#include <string>

namespace SeaBattle::Network
{
    // Callback types for network events
    using MessageReceivedCallback = std::function<void(const Message&)>;
    using ConnectionEstablishedCallback = std::function<void()>;
    using ConnectionClosedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string&)>;

    enum class ConnectionStatus
    {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

    // Base class for network sessions (independent of Qt)
    class NetworkSession : public std::enable_shared_from_this<NetworkSession>
    {
    public:
        NetworkSession(boost::asio::io_context& ioContext)
            : m_ioContext(ioContext)
            , m_socket(ioContext)
            , m_status(ConnectionStatus::Disconnected)
        {
        }

        virtual ~NetworkSession() = default;

        // Send a message
        void sendMessage(const Message& message)
        {
            auto msg = message.serialize() + "\n"; // Add newline as delimiter
            
            boost::asio::post(m_ioContext, [this, self = shared_from_this(), msg]() {
                bool writeInProgress = !m_writeQueue.empty();
                m_writeQueue.push(msg);
                if (!writeInProgress)
                {
                    doWrite();
                }
            });
        }

        // Close connection
        void close()
        {
            boost::asio::post(m_ioContext, [this, self = shared_from_this()]() {
                if (m_socket.is_open())
                {
                    boost::system::error_code ec;
                    m_socket.close(ec);
                }
                m_status = ConnectionStatus::Disconnected;
            });
        }

        // Set callbacks
        void setMessageReceivedCallback(MessageReceivedCallback callback)
        {
            m_messageReceivedCallback = std::move(callback);
        }

        void setConnectionEstablishedCallback(ConnectionEstablishedCallback callback)
        {
            m_connectionEstablishedCallback = std::move(callback);
        }

        void setConnectionClosedCallback(ConnectionClosedCallback callback)
        {
            m_connectionClosedCallback = std::move(callback);
        }

        void setErrorCallback(ErrorCallback callback)
        {
            m_errorCallback = std::move(callback);
        }

        ConnectionStatus getStatus() const { return m_status; }

    protected:
        // Start reading messages
        void startReading()
        {
            doRead();
        }

        boost::asio::ip::tcp::socket& socket() { return m_socket; }

        void setStatus(ConnectionStatus status)
        {
            m_status = status;
        }

        void notifyConnectionEstablished()
        {
            m_status = ConnectionStatus::Connected;
            if (m_connectionEstablishedCallback)
            {
                m_connectionEstablishedCallback();
            }
        }

        void notifyError(const std::string& error)
        {
            m_status = ConnectionStatus::Error;
            if (m_errorCallback)
            {
                m_errorCallback(error);
            }
        }

    private:
        static constexpr size_t MAX_MESSAGE_SIZE = 65536; // 64KB max message size
        
        void doRead()
        {
            auto self = shared_from_this();
            boost::asio::async_read_until(m_socket, m_readBuffer, '\n',
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec)
                    {
                        // Check message size to prevent unbounded memory growth
                        if (length > MAX_MESSAGE_SIZE)
                        {
                            notifyError("Message too large");
                            close();
                            return;
                        }
                        
                        std::istream is(&m_readBuffer);
                        std::string line;
                        std::getline(is, line);

                        try
                        {
                            Message msg = Message::deserialize(line);
                            if (m_messageReceivedCallback)
                            {
                                m_messageReceivedCallback(msg);
                            }
                        }
                        catch (const std::exception& e)
                        {
                            notifyError(std::string("Failed to parse message: ") + e.what());
                        }

                        doRead();
                    }
                    else
                    {
                        if (ec != boost::asio::error::operation_aborted)
                        {
                            if (m_connectionClosedCallback)
                            {
                                m_connectionClosedCallback();
                            }
                        }
                        close();
                    }
                });
        }

        void doWrite()
        {
            auto self = shared_from_this();
            boost::asio::async_write(m_socket,
                boost::asio::buffer(m_writeQueue.front()),
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec)
                    {
                        m_writeQueue.pop();
                        if (!m_writeQueue.empty())
                        {
                            doWrite();
                        }
                    }
                    else
                    {
                        notifyError("Failed to send message: " + ec.message());
                        close();
                    }
                });
        }

    protected:
        boost::asio::io_context& m_ioContext;
        boost::asio::ip::tcp::socket m_socket;
        boost::asio::streambuf m_readBuffer;
        std::queue<std::string> m_writeQueue;
        ConnectionStatus m_status;

        MessageReceivedCallback m_messageReceivedCallback;
        ConnectionEstablishedCallback m_connectionEstablishedCallback;
        ConnectionClosedCallback m_connectionClosedCallback;
        ErrorCallback m_errorCallback;
    };

} // namespace SeaBattle::Network
