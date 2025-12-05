#pragma once

#include "NetworkSession.h"
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace SeaBattle::Network
{
    // Client session - connects to server
    class ClientSession : public NetworkSession
    {
    public:
        ClientSession(boost::asio::io_context& ioContext)
            : NetworkSession(ioContext)
        {
        }

        // Connect to server
        void connect(const std::string& host, unsigned short port)
        {
            setStatus(ConnectionStatus::Connecting);
            
            boost::asio::ip::tcp::resolver resolver(m_ioContext);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            boost::asio::async_connect(m_socket, endpoints,
                [this, self = shared_from_this()](boost::system::error_code ec,
                    boost::asio::ip::tcp::endpoint /*endpoint*/) {
                    if (!ec)
                    {
                        notifyConnectionEstablished();
                        startReading();
                    }
                    else
                    {
                        notifyError("Failed to connect: " + ec.message());
                    }
                });
        }
    };

} // namespace SeaBattle::Network
