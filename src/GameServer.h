#pragma once

#include "Model.h"
#include "GameProtocol.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <functional>
#include <random>

namespace SeaBattle
{
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    using tcp = asio::ip::tcp;

    // Game session state
    enum class SessionState
    {
        WaitingForClient,
        ClientConnected,
        WaitingForReady,
        GameInProgress,
        GameOver,
        Closed
    };

    // Game session manages one client connection and game logic
    class GameSession : public std::enable_shared_from_this<GameSession>
    {
    public:
        GameSession(tcp::socket socket);
        
        void start();
        void close();
        
        SessionState getState() const { return state_; }

    private:
        void readMessage();
        void handleMessage(const std::string& message);
        void sendMessage(const std::string& message);
        void sendMessage(const Protocol::Message& message);
        
        void processJoinGame(const std::string& playerName);
        void processReady();
        void processShoot(int row, int col);
        void processQuit();
        
        Protocol::ShotResult mapCellStateToShotResult(CellState cellState) const;
        
        tcp::socket socket_;
        asio::streambuf buffer_;
        std::unique_ptr<GameModel> gameModel_;
        SessionState state_;
        std::string playerName_;
        bool clientReady_;
        std::random_device rd_;
        std::mt19937 gen_;
    };

    // Game server using boost::asio
    class GameServer
    {
    public:
        explicit GameServer(asio::io_context& ioc, unsigned short port);
        
        void start();
        void stop();
        
        bool isRunning() const { return running_; }
        std::shared_ptr<GameSession> getCurrentSession() const { return currentSession_; }

    private:
        void startAccept();
        void handleAccept(boost::system::error_code ec, tcp::socket socket);
        
        asio::io_context& ioc_;
        tcp::acceptor acceptor_;
        bool running_;
        std::shared_ptr<GameSession> currentSession_;
    };

} // namespace SeaBattle
