#include "GameServer.h"
#include <iostream>
#include <istream>

namespace SeaBattle
{
    // GameSession implementation
    
    GameSession::GameSession(tcp::socket socket)
        : socket_(std::move(socket))
        , state_(SessionState::WaitingForClient)
        , clientReady_(false)
        , gen_(rd_())
    {
        gameModel_ = std::make_unique<GameModel>();
    }

    void GameSession::start()
    {
        state_ = SessionState::ClientConnected;
        std::cout << "[Server] Client connected" << std::endl;
        readMessage();
    }

    void GameSession::close()
    {
        state_ = SessionState::Closed;
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        std::cout << "[Server] Session closed" << std::endl;
    }

    void GameSession::readMessage()
    {
        auto self = shared_from_this();
        
        asio::async_read_until(
            socket_,
            buffer_,
            '\n',
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    // Extract message from buffer using istream
                    std::istream is(&buffer_);
                    std::string message;
                    std::getline(is, message);
                    
                    std::cout << "[Server] Received: " << message << std::endl;
                    handleMessage(message);
                    
                    // Continue reading messages unless session is closed
                    if (state_ != SessionState::Closed)
                    {
                        readMessage();
                    }
                }
                else
                {
                    std::cout << "[Server] Read error: " << ec.message() << std::endl;
                    close();
                }
            });
    }

    void GameSession::handleMessage(const std::string& message)
    {
        auto parsedMsg = Protocol::MessageParser::parse(message);
        
        if (!parsedMsg)
        {
            Protocol::ErrorMessage error("Invalid message format");
            sendMessage(error);
            return;
        }
        
        switch (parsedMsg->type)
        {
            case Protocol::MessageType::JOIN_GAME:
            {
                auto* joinMsg = static_cast<Protocol::JoinGameMessage*>(parsedMsg.get());
                processJoinGame(joinMsg->playerName);
                break;
            }
            
            case Protocol::MessageType::READY:
                processReady();
                break;
            
            case Protocol::MessageType::SHOOT:
            {
                auto* shootMsg = static_cast<Protocol::ShootMessage*>(parsedMsg.get());
                processShoot(shootMsg->row, shootMsg->col);
                break;
            }
            
            case Protocol::MessageType::QUIT:
                processQuit();
                break;
            
            default:
                Protocol::ErrorMessage error("Unexpected message type");
                sendMessage(error);
                break;
        }
    }

    void GameSession::sendMessage(const std::string& message)
    {
        auto self = shared_from_this();
        
        auto buffer = std::make_shared<std::string>(message);
        
        asio::async_write(
            socket_,
            asio::buffer(*buffer),
            [this, self, buffer](boost::system::error_code ec, std::size_t /*bytes_transferred*/)
            {
                if (ec)
                {
                    std::cout << "[Server] Write error: " << ec.message() << std::endl;
                    close();
                }
            });
    }

    void GameSession::sendMessage(const Protocol::Message& message)
    {
        sendMessage(message.serialize());
    }

    void GameSession::processJoinGame(const std::string& playerName)
    {
        if (state_ != SessionState::ClientConnected)
        {
            std::string errorMsg;
            switch (state_) {
                case SessionState::WaitingForReady:
                    errorMsg = "Already joined. Send READY to start the game.";
                    break;
                case SessionState::GameInProgress:
                    errorMsg = "Cannot join - game is already in progress";
                    break;
                default:
                    errorMsg = "Cannot join at this state";
                    break;
            }
            Protocol::ErrorMessage error(errorMsg);
            sendMessage(error);
            return;
        }
        
        playerName_ = playerName;
        std::cout << "[Server] Player joined: " << playerName_ << std::endl;
        
        // Assign player as player 0
        Protocol::GameStartedMessage gameStarted(0);
        sendMessage(gameStarted);
        
        // Transition to waiting for ready state
        state_ = SessionState::WaitingForReady;
    }

    void GameSession::processReady()
    {
        if (state_ != SessionState::WaitingForReady)
        {
            std::string errorMsg;
            switch (state_) {
                case SessionState::ClientConnected:
                    errorMsg = "Must join game before sending READY";
                    break;
                case SessionState::GameInProgress:
                    errorMsg = "Game already in progress";
                    break;
                default:
                    errorMsg = "Cannot ready at this state";
                    break;
            }
            Protocol::ErrorMessage error(errorMsg);
            sendMessage(error);
            return;
        }
        
        clientReady_ = true;
        std::cout << "[Server] Client ready, starting game" << std::endl;
        
        // Start the game
        gameModel_->startGame();
        state_ = SessionState::GameInProgress;
        
        // Player 0 starts first
        Protocol::YourTurnMessage yourTurn;
        sendMessage(yourTurn);
    }

    void GameSession::processShoot(int row, int col)
    {
        if (state_ != SessionState::GameInProgress)
        {
            Protocol::ErrorMessage error("Game is not in progress");
            sendMessage(error);
            return;
        }
        
        if (gameModel_->getCurrentPlayer() != 0)
        {
            Protocol::ErrorMessage error("Not your turn");
            sendMessage(error);
            return;
        }
        
        if (!gameModel_->isValidShot(row, col))
        {
            Protocol::ShootResultMessage result(row, col, Protocol::ShotResult::INVALID);
            sendMessage(result);
            return;
        }
        
        gameModel_->shoot(row, col);
        
        // Determine result
        CellState cellState = gameModel_->getEnemyCellState(0, row, col);
        Protocol::ShotResult result = mapCellStateToShotResult(cellState);
        
        Protocol::ShootResultMessage shootResult(row, col, result);
        sendMessage(shootResult);
        
        // Check if game is over
        if (gameModel_->getGameState() == GameState::GameOver)
        {
            Protocol::GameOverMessage gameOver(gameModel_->getWinner());
            sendMessage(gameOver);
            state_ = SessionState::GameOver;
            std::cout << "[Server] Game over, winner: " << gameModel_->getWinner() << std::endl;
        }
        else if (gameModel_->getCurrentPlayer() == 0)
        {
            // Still player's turn (hit)
            Protocol::YourTurnMessage yourTurn;
            sendMessage(yourTurn);
        }
        else
        {
            // Simulate AI opponent's turn
            std::cout << "[Server] AI opponent's turn" << std::endl;
            
            // Simple AI: random shots
            std::uniform_int_distribution<> dis(0, GameField::SIZE - 1);
            
            bool aiContinues = true;
            while (aiContinues && gameModel_->getGameState() == GameState::Playing)
            {
                int aiRow = dis(gen_);
                int aiCol = dis(gen_);
                
                if (gameModel_->isValidShot(aiRow, aiCol))
                {
                    bool aiHit = gameModel_->shoot(aiRow, aiCol);
                    
                    CellState aiCellState = gameModel_->getEnemyCellState(1, aiRow, aiCol);
                    Protocol::ShotResult aiResult = mapCellStateToShotResult(aiCellState);
                    
                    Protocol::OpponentShotMessage opponentShot(aiRow, aiCol, aiResult);
                    sendMessage(opponentShot);
                    
                    if (gameModel_->getGameState() == GameState::GameOver)
                    {
                        Protocol::GameOverMessage gameOver(gameModel_->getWinner());
                        sendMessage(gameOver);
                        state_ = SessionState::GameOver;
                        std::cout << "[Server] Game over, winner: " << gameModel_->getWinner() << std::endl;
                        aiContinues = false;
                    }
                    else if (!aiHit)
                    {
                        // AI missed, back to player's turn
                        aiContinues = false;
                        Protocol::YourTurnMessage yourTurn;
                        sendMessage(yourTurn);
                    }
                    // else: AI hit, continue
                }
            }
        }
    }

    void GameSession::processQuit()
    {
        std::cout << "[Server] Client quit" << std::endl;
        close();
    }
    
    Protocol::ShotResult GameSession::mapCellStateToShotResult(CellState cellState) const
    {
        switch (cellState)
        {
            case CellState::Destroyed:
                return Protocol::ShotResult::DESTROYED;
            case CellState::Hit:
                return Protocol::ShotResult::HIT;
            case CellState::Miss:
                return Protocol::ShotResult::MISS;
            default:
                return Protocol::ShotResult::INVALID;
        }
    }

    // GameServer implementation
    
    GameServer::GameServer(asio::io_context& ioc, unsigned short port)
        : ioc_(ioc)
        , acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
        , running_(false)
    {
        std::cout << "[Server] Server created on port " << port << std::endl;
    }

    void GameServer::start()
    {
        running_ = true;
        std::cout << "[Server] Server started, waiting for connections..." << std::endl;
        startAccept();
    }

    void GameServer::stop()
    {
        running_ = false;
        
        if (currentSession_)
        {
            currentSession_->close();
        }
        
        boost::system::error_code ec;
        acceptor_.close(ec);
        
        std::cout << "[Server] Server stopped" << std::endl;
    }

    void GameServer::startAccept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                handleAccept(ec, std::move(socket));
            });
    }

    void GameServer::handleAccept(boost::system::error_code ec, tcp::socket socket)
    {
        if (!ec)
        {
            // Only one client at a time
            if (currentSession_ && currentSession_->getState() != SessionState::Closed &&
                currentSession_->getState() != SessionState::GameOver)
            {
                std::cout << "[Server] Rejecting connection - session in progress" << std::endl;
                try {
                    const std::string msg = "ERROR Server busy - another session in progress\n";
                    asio::write(socket, asio::buffer(msg));
                } catch (const std::exception& e) {
                    // Ignore errors when sending rejection message (client may have disconnected)
                    std::cout << "[Server] Failed to send rejection message: " << e.what() << std::endl;
                }
                socket.close();
            }
            else
            {
                currentSession_ = std::make_shared<GameSession>(std::move(socket));
                currentSession_->start();
            }
        }
        else
        {
            std::cout << "[Server] Accept error: " << ec.message() << std::endl;
        }
        
        if (running_)
        {
            startAccept();
        }
    }

} // namespace SeaBattle
