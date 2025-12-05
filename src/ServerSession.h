#pragma once

#include "NetworkSession.h"
#include <boost/asio.hpp>
#include <memory>

namespace SeaBattle::Network
{
    // Server session - accepts one client connection
    class ServerSession
    {
    public:
        using ClientConnectedCallback = std::function<void(std::shared_ptr<NetworkSession>)>;

        ServerSession(boost::asio::io_context& ioContext, unsigned short port)
            : m_ioContext(ioContext)
            , m_acceptor(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
            , m_port(port)
        {
        }

        // Start accepting connections
        void startAccepting()
        {
            doAccept();
        }

        // Stop accepting and close
        void stop()
        {
            boost::system::error_code ec;
            m_acceptor.close(ec);
            if (m_clientSession)
            {
                m_clientSession->close();
            }
        }

        void setClientConnectedCallback(ClientConnectedCallback callback)
        {
            m_clientConnectedCallback = std::move(callback);
        }

        void setErrorCallback(ErrorCallback callback)
        {
            m_errorCallback = std::move(callback);
        }

        unsigned short getPort() const { return m_port; }

    private:
        void doAccept()
        {
            m_clientSession = std::make_shared<NetworkSession>(m_ioContext);
            
            m_acceptor.async_accept(m_clientSession->socket(),
                [this](boost::system::error_code ec) {
                    if (!ec)
                    {
                        m_clientSession->notifyConnectionEstablished();
                        m_clientSession->startReading();
                        
                        if (m_clientConnectedCallback)
                        {
                            m_clientConnectedCallback(m_clientSession);
                        }
                    }
                    else
                    {
                        if (m_errorCallback)
                        {
                            m_errorCallback("Failed to accept client: " + ec.message());
                        }
                    }
                });
        }

        boost::asio::io_context& m_ioContext;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::shared_ptr<NetworkSession> m_clientSession;
        unsigned short m_port;

        ClientConnectedCallback m_clientConnectedCallback;
        ErrorCallback m_errorCallback;
    };

} // namespace SeaBattle::Network
