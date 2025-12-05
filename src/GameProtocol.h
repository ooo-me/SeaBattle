#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <optional>
#include <memory>

namespace SeaBattle::Protocol
{
    // Message types for client-server communication
    enum class MessageType
    {
        // Client -> Server
        JOIN_GAME,      // Client wants to join the game
        READY,          // Client is ready to start
        SHOOT,          // Client shoots at coordinates
        QUIT,           // Client quits the game
        
        // Server -> Client
        GAME_STARTED,   // Game has started
        YOUR_TURN,      // It's your turn to shoot
        SHOOT_RESULT,   // Result of a shot (hit/miss)
        OPPONENT_SHOT,  // Opponent shot at these coordinates
        GAME_OVER,      // Game is over
        ERROR_MSG       // Error message
    };

    // Shot result types
    enum class ShotResult
    {
        MISS,
        HIT,
        DESTROYED,
        INVALID
    };

    // Base message structure
    struct Message
    {
        MessageType type;
        
        virtual ~Message() = default;
        virtual std::string serialize() const = 0;
        
        static std::string messageTypeToString(MessageType type)
        {
            switch (type)
            {
                case MessageType::JOIN_GAME: return "JOIN_GAME";
                case MessageType::READY: return "READY";
                case MessageType::SHOOT: return "SHOOT";
                case MessageType::QUIT: return "QUIT";
                case MessageType::GAME_STARTED: return "GAME_STARTED";
                case MessageType::YOUR_TURN: return "YOUR_TURN";
                case MessageType::SHOOT_RESULT: return "SHOOT_RESULT";
                case MessageType::OPPONENT_SHOT: return "OPPONENT_SHOT";
                case MessageType::GAME_OVER: return "GAME_OVER";
                case MessageType::ERROR_MSG: return "ERROR";
                default: return "UNKNOWN";
            }
        }
        
        static std::optional<MessageType> stringToMessageType(const std::string& str)
        {
            if (str == "JOIN_GAME") return MessageType::JOIN_GAME;
            if (str == "READY") return MessageType::READY;
            if (str == "SHOOT") return MessageType::SHOOT;
            if (str == "QUIT") return MessageType::QUIT;
            if (str == "GAME_STARTED") return MessageType::GAME_STARTED;
            if (str == "YOUR_TURN") return MessageType::YOUR_TURN;
            if (str == "SHOOT_RESULT") return MessageType::SHOOT_RESULT;
            if (str == "OPPONENT_SHOT") return MessageType::OPPONENT_SHOT;
            if (str == "GAME_OVER") return MessageType::GAME_OVER;
            if (str == "ERROR") return MessageType::ERROR_MSG;
            return std::nullopt;
        }
    };

    // Client wants to join
    struct JoinGameMessage : Message
    {
        std::string playerName;
        
        JoinGameMessage(std::string name = "Player")
            : playerName(std::move(name))
        {
            type = MessageType::JOIN_GAME;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + " " + playerName + "\n";
        }
    };

    // Client is ready
    struct ReadyMessage : Message
    {
        ReadyMessage()
        {
            type = MessageType::READY;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + "\n";
        }
    };

    // Client shoots
    struct ShootMessage : Message
    {
        int row;
        int col;
        
        ShootMessage(int r = 0, int c = 0)
            : row(r), col(c)
        {
            type = MessageType::SHOOT;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + " " + std::to_string(row) + " " + std::to_string(col) + "\n";
        }
    };

    // Client quits
    struct QuitMessage : Message
    {
        QuitMessage()
        {
            type = MessageType::QUIT;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + "\n";
        }
    };

    // Game started
    struct GameStartedMessage : Message
    {
        int playerNumber; // 0 or 1
        
        GameStartedMessage(int pn = 0)
            : playerNumber(pn)
        {
            type = MessageType::GAME_STARTED;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + " " + std::to_string(playerNumber) + "\n";
        }
    };

    // Your turn
    struct YourTurnMessage : Message
    {
        YourTurnMessage()
        {
            type = MessageType::YOUR_TURN;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + "\n";
        }
    };

    // Shoot result
    struct ShootResultMessage : Message
    {
        int row;
        int col;
        ShotResult result;
        
        ShootResultMessage(int r = 0, int c = 0, ShotResult res = ShotResult::MISS)
            : row(r), col(c), result(res)
        {
            type = MessageType::SHOOT_RESULT;
        }
        
        std::string serialize() const override
        {
            std::string resultStr;
            switch (result)
            {
                case ShotResult::MISS: resultStr = "MISS"; break;
                case ShotResult::HIT: resultStr = "HIT"; break;
                case ShotResult::DESTROYED: resultStr = "DESTROYED"; break;
                case ShotResult::INVALID: resultStr = "INVALID"; break;
            }
            return messageTypeToString(type) + " " + std::to_string(row) + " " + std::to_string(col) + " " + resultStr + "\n";
        }
    };

    // Opponent shot
    struct OpponentShotMessage : Message
    {
        int row;
        int col;
        ShotResult result;
        
        OpponentShotMessage(int r = 0, int c = 0, ShotResult res = ShotResult::MISS)
            : row(r), col(c), result(res)
        {
            type = MessageType::OPPONENT_SHOT;
        }
        
        std::string serialize() const override
        {
            std::string resultStr;
            switch (result)
            {
                case ShotResult::MISS: resultStr = "MISS"; break;
                case ShotResult::HIT: resultStr = "HIT"; break;
                case ShotResult::DESTROYED: resultStr = "DESTROYED"; break;
                case ShotResult::INVALID: resultStr = "INVALID"; break;
            }
            return messageTypeToString(type) + " " + std::to_string(row) + " " + std::to_string(col) + " " + resultStr + "\n";
        }
    };

    // Game over
    struct GameOverMessage : Message
    {
        int winner; // Player number who won
        
        GameOverMessage(int w = 0)
            : winner(w)
        {
            type = MessageType::GAME_OVER;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + " " + std::to_string(winner) + "\n";
        }
    };

    // Error message
    struct ErrorMessage : Message
    {
        std::string errorText;
        
        ErrorMessage(std::string text = "")
            : errorText(std::move(text))
        {
            type = MessageType::ERROR_MSG;
        }
        
        std::string serialize() const override
        {
            return messageTypeToString(type) + " " + errorText + "\n";
        }
    };

    // Message parser
    class MessageParser
    {
    public:
        static std::unique_ptr<Message> parse(const std::string& data)
        {
            std::istringstream iss(data);
            std::string typeStr;
            iss >> typeStr;
            
            auto messageType = Message::stringToMessageType(typeStr);
            if (!messageType)
            {
                return nullptr;
            }
            
            switch (*messageType)
            {
                case MessageType::JOIN_GAME:
                {
                    std::string name;
                    std::getline(iss >> std::ws, name);
                    return std::make_unique<JoinGameMessage>(name);
                }
                
                case MessageType::READY:
                    return std::make_unique<ReadyMessage>();
                
                case MessageType::SHOOT:
                {
                    int row, col;
                    if (iss >> row >> col)
                    {
                        return std::make_unique<ShootMessage>(row, col);
                    }
                    return nullptr;
                }
                
                case MessageType::QUIT:
                    return std::make_unique<QuitMessage>();
                
                case MessageType::GAME_STARTED:
                {
                    int playerNum;
                    if (iss >> playerNum)
                    {
                        return std::make_unique<GameStartedMessage>(playerNum);
                    }
                    return nullptr;
                }
                
                case MessageType::YOUR_TURN:
                    return std::make_unique<YourTurnMessage>();
                
                case MessageType::SHOOT_RESULT:
                {
                    int row, col;
                    std::string resultStr;
                    if (iss >> row >> col >> resultStr)
                    {
                        ShotResult result = ShotResult::INVALID;
                        if (resultStr == "MISS") result = ShotResult::MISS;
                        else if (resultStr == "HIT") result = ShotResult::HIT;
                        else if (resultStr == "DESTROYED") result = ShotResult::DESTROYED;
                        
                        return std::make_unique<ShootResultMessage>(row, col, result);
                    }
                    return nullptr;
                }
                
                case MessageType::OPPONENT_SHOT:
                {
                    int row, col;
                    std::string resultStr;
                    if (iss >> row >> col >> resultStr)
                    {
                        ShotResult result = ShotResult::INVALID;
                        if (resultStr == "MISS") result = ShotResult::MISS;
                        else if (resultStr == "HIT") result = ShotResult::HIT;
                        else if (resultStr == "DESTROYED") result = ShotResult::DESTROYED;
                        
                        return std::make_unique<OpponentShotMessage>(row, col, result);
                    }
                    return nullptr;
                }
                
                case MessageType::GAME_OVER:
                {
                    int winner;
                    if (iss >> winner)
                    {
                        return std::make_unique<GameOverMessage>(winner);
                    }
                    return nullptr;
                }
                
                case MessageType::ERROR_MSG:
                {
                    std::string errorText;
                    std::getline(iss >> std::ws, errorText);
                    return std::make_unique<ErrorMessage>(errorText);
                }
                
                default:
                    return nullptr;
            }
        }
    };

} // namespace SeaBattle::Protocol
