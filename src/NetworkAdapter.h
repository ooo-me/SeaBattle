#pragma once

#include "Protocol.h"
#include "NetworkSession.h"
#include "ServerSession.h"
#include "ClientSession.h"
#include <QObject>
#include <QThread>
#include <memory>
#include <boost/asio.hpp>

namespace SeaBattle
{
    // Qt adapter for network layer - bridges network events to Qt signals
    class NetworkAdapter : public QObject
    {
        Q_OBJECT

    public:
        enum class Role
        {
            Server,
            Client
        };

        NetworkAdapter(QObject* parent = nullptr)
            : QObject(parent)
            , m_ioContext()
            , m_work(boost::asio::make_work_guard(m_ioContext))
            , m_role(Role::Client)
        {
            // Start IO context in a separate thread
            m_ioThread = std::make_unique<std::thread>([this]() {
                m_ioContext.run();
            });
        }

        ~NetworkAdapter()
        {
            stop();
        }

        void startServer(unsigned short port)
        {
            m_role = Role::Server;
            m_serverSession = std::make_unique<Network::ServerSession>(m_ioContext, port);
            
            m_serverSession->setClientConnectedCallback([this](std::shared_ptr<Network::NetworkSession> session) {
                m_session = session;
                setupSessionCallbacks();
                emit connectionEstablished();
            });

            m_serverSession->setErrorCallback([this](const std::string& error) {
                emit errorOccurred(QString::fromStdString(error));
            });

            m_serverSession->startAccepting();
        }

        void connectToServer(const std::string& host, unsigned short port)
        {
            m_role = Role::Client;
            auto clientSession = std::make_shared<Network::ClientSession>(m_ioContext);
            m_session = clientSession;
            
            setupSessionCallbacks();
            clientSession->connect(host, port);
        }

        void sendMessage(const Network::Message& message)
        {
            if (m_session)
            {
                m_session->sendMessage(message);
            }
        }

        void stop()
        {
            if (m_serverSession)
            {
                m_serverSession->stop();
            }
            if (m_session)
            {
                m_session->close();
            }
            
            m_work.reset();
            
            if (m_ioThread && m_ioThread->joinable())
            {
                m_ioContext.stop();
                m_ioThread->join();
            }
        }

        Role getRole() const { return m_role; }

    signals:
        void messageReceived(const Network::Message& message);
        void connectionEstablished();
        void connectionClosed();
        void errorOccurred(const QString& error);

    private:
        void setupSessionCallbacks()
        {
            m_session->setMessageReceivedCallback([this](const Network::Message& msg) {
                emit messageReceived(msg);
            });

            m_session->setConnectionEstablishedCallback([this]() {
                emit connectionEstablished();
            });

            m_session->setConnectionClosedCallback([this]() {
                emit connectionClosed();
            });

            m_session->setErrorCallback([this](const std::string& error) {
                emit errorOccurred(QString::fromStdString(error));
            });
        }

        boost::asio::io_context m_ioContext;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work;
        std::unique_ptr<std::thread> m_ioThread;
        
        std::unique_ptr<Network::ServerSession> m_serverSession;
        std::shared_ptr<Network::NetworkSession> m_session;
        
        Role m_role;
    };

} // namespace SeaBattle
