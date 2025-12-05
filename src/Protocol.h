#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <variant>

namespace SeaBattle::Network
{
    // Message types for network protocol
    enum class MessageType
    {
        // Game messages
        Shot,           // Shot request from current player
        ShotResult,     // Result of a shot (hit/miss/destroyed)
        GameStart,      // Game started notification
        GameOver,       // Game ended notification
        PlayerSwitch,   // Turn switched to other player
        
        // Connection messages
        Connect,        // Client connection request
        ConnectAccept,  // Server accepts connection
        Disconnect,     // Clean disconnect
        
        // Status messages
        Error,          // Error notification
        
        // Future extensibility
        ChatMessage     // Placeholder for chat functionality
    };

    // Convert MessageType to string
    inline std::string messageTypeToString(MessageType type)
    {
        switch (type)
        {
            case MessageType::Shot: return "Shot";
            case MessageType::ShotResult: return "ShotResult";
            case MessageType::GameStart: return "GameStart";
            case MessageType::GameOver: return "GameOver";
            case MessageType::PlayerSwitch: return "PlayerSwitch";
            case MessageType::Connect: return "Connect";
            case MessageType::ConnectAccept: return "ConnectAccept";
            case MessageType::Disconnect: return "Disconnect";
            case MessageType::Error: return "Error";
            case MessageType::ChatMessage: return "ChatMessage";
            default: return "Unknown";
        }
    }

    // Parse MessageType from string
    inline MessageType stringToMessageType(const std::string& str)
    {
        if (str == "Shot") return MessageType::Shot;
        if (str == "ShotResult") return MessageType::ShotResult;
        if (str == "GameStart") return MessageType::GameStart;
        if (str == "GameOver") return MessageType::GameOver;
        if (str == "PlayerSwitch") return MessageType::PlayerSwitch;
        if (str == "Connect") return MessageType::Connect;
        if (str == "ConnectAccept") return MessageType::ConnectAccept;
        if (str == "Disconnect") return MessageType::Disconnect;
        if (str == "Error") return MessageType::Error;
        if (str == "ChatMessage") return MessageType::ChatMessage;
        return MessageType::Error;
    }

    // Base message structure
    struct Message
    {
        MessageType type;
        nlohmann::json payload;

        std::string serialize() const
        {
            nlohmann::json j;
            j["type"] = messageTypeToString(type);
            j["payload"] = payload;
            return j.dump();
        }

        static Message deserialize(const std::string& str)
        {
            nlohmann::json j = nlohmann::json::parse(str);
            Message msg;
            msg.type = stringToMessageType(j["type"].get<std::string>());
            msg.payload = j["payload"];
            return msg;
        }
    };

    // Specific message payloads
    struct ShotMessage
    {
        int row;
        int col;

        nlohmann::json toJson() const
        {
            return {{"row", row}, {"col", col}};
        }

        static ShotMessage fromJson(const nlohmann::json& j)
        {
            return {j["row"].get<int>(), j["col"].get<int>()};
        }
    };

    struct ShotResultMessage
    {
        int row;
        int col;
        bool hit;
        bool destroyed;

        nlohmann::json toJson() const
        {
            return {{"row", row}, {"col", col}, {"hit", hit}, {"destroyed", destroyed}};
        }

        static ShotResultMessage fromJson(const nlohmann::json& j)
        {
            return {
                j["row"].get<int>(),
                j["col"].get<int>(),
                j["hit"].get<bool>(),
                j["destroyed"].get<bool>()
            };
        }
    };

    struct GameStartMessage
    {
        bool isServer;

        nlohmann::json toJson() const
        {
            return {{"isServer", isServer}};
        }

        static GameStartMessage fromJson(const nlohmann::json& j)
        {
            return {j["isServer"].get<bool>()};
        }
    };

    struct GameOverMessage
    {
        int winner; // 0 for server, 1 for client

        nlohmann::json toJson() const
        {
            return {{"winner", winner}};
        }

        static GameOverMessage fromJson(const nlohmann::json& j)
        {
            return {j["winner"].get<int>()};
        }
    };

    struct ErrorMessage
    {
        std::string message;

        nlohmann::json toJson() const
        {
            return {{"message", message}};
        }

        static ErrorMessage fromJson(const nlohmann::json& j)
        {
            return {j["message"].get<std::string>()};
        }
    };

    struct ChatMessage
    {
        std::string sender;
        std::string message;

        nlohmann::json toJson() const
        {
            return {{"sender", sender}, {"message", message}};
        }

        static ChatMessage fromJson(const nlohmann::json& j)
        {
            return {j["sender"].get<std::string>(), j["message"].get<std::string>()};
        }
    };

    // Helper functions to create messages
    inline Message createShotMessage(int row, int col)
    {
        ShotMessage shot{row, col};
        return {MessageType::Shot, shot.toJson()};
    }

    inline Message createShotResultMessage(int row, int col, bool hit, bool destroyed)
    {
        ShotResultMessage result{row, col, hit, destroyed};
        return {MessageType::ShotResult, result.toJson()};
    }

    inline Message createGameStartMessage(bool isServer)
    {
        GameStartMessage start{isServer};
        return {MessageType::GameStart, start.toJson()};
    }

    inline Message createGameOverMessage(int winner)
    {
        GameOverMessage gameOver{winner};
        return {MessageType::GameOver, gameOver.toJson()};
    }

    inline Message createErrorMessage(const std::string& message)
    {
        ErrorMessage error{message};
        return {MessageType::Error, error.toJson()};
    }

    inline Message createConnectMessage()
    {
        return {MessageType::Connect, nlohmann::json::object()};
    }

    inline Message createConnectAcceptMessage()
    {
        return {MessageType::ConnectAccept, nlohmann::json::object()};
    }

    inline Message createDisconnectMessage()
    {
        return {MessageType::Disconnect, nlohmann::json::object()};
    }

} // namespace SeaBattle::Network
