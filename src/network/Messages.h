#pragma once

#include "IMessage.h"
#include <optional>

namespace SeaBattle::Network
{
    /**
     * @brief Shot message - player taking a shot at opponent's field
     */
    class ShotMessage : public BaseMessage
    {
    public:
        ShotMessage(int row, int col)
            : BaseMessage("shot")
            , m_row(row)
            , m_col(col)
        {
        }

        static std::unique_ptr<ShotMessage> fromJson(const nlohmann::json& json);

        int getRow() const { return m_row; }
        int getCol() const { return m_col; }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["row"] = m_row;
            payload["col"] = m_col;
            return payload;
        }

        bool validatePayload() const override
        {
            return m_row >= 0 && m_row < 10 && m_col >= 0 && m_col < 10;
        }

    private:
        int m_row;
        int m_col;
    };

    /**
     * @brief Shot response message - result of a shot
     */
    class ShotResponseMessage : public BaseMessage
    {
    public:
        enum class Result
        {
            Miss,
            Hit,
            Destroyed
        };

        ShotResponseMessage(int row, int col, Result result, bool gameOver = false, 
                           std::optional<int> winner = std::nullopt)
            : BaseMessage("shot_response")
            , m_row(row)
            , m_col(col)
            , m_result(result)
            , m_gameOver(gameOver)
            , m_winner(winner)
        {
        }

        static std::unique_ptr<ShotResponseMessage> fromJson(const nlohmann::json& json);

        int getRow() const { return m_row; }
        int getCol() const { return m_col; }
        Result getResult() const { return m_result; }
        bool isGameOver() const { return m_gameOver; }
        std::optional<int> getWinner() const { return m_winner; }

        static std::string resultToString(Result result)
        {
            switch (result)
            {
            case Result::Miss: return "miss";
            case Result::Hit: return "hit";
            case Result::Destroyed: return "destroyed";
            default: return "unknown";
            }
        }

        static Result stringToResult(const std::string& str)
        {
            if (str == "miss") return Result::Miss;
            if (str == "hit") return Result::Hit;
            if (str == "destroyed") return Result::Destroyed;
            throw std::invalid_argument("Invalid result string: " + str);
        }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["row"] = m_row;
            payload["col"] = m_col;
            payload["result"] = resultToString(m_result);
            payload["game_over"] = m_gameOver;
            if (m_winner.has_value())
            {
                payload["winner"] = m_winner.value();
            }
            else
            {
                payload["winner"] = nullptr;
            }
            return payload;
        }

        bool validatePayload() const override
        {
            bool coordsValid = m_row >= 0 && m_row < 10 && m_col >= 0 && m_col < 10;
            bool winnerValid = !m_gameOver || m_winner.has_value();
            return coordsValid && (!m_gameOver || winnerValid);
        }

    private:
        int m_row;
        int m_col;
        Result m_result;
        bool m_gameOver;
        std::optional<int> m_winner;
    };

    /**
     * @brief Error message - indicates an error occurred
     */
    class ErrorMessage : public BaseMessage
    {
    public:
        ErrorMessage(std::string code, std::string message, 
                    nlohmann::json details = nlohmann::json::object())
            : BaseMessage("error")
            , m_code(std::move(code))
            , m_message(std::move(message))
            , m_details(std::move(details))
        {
        }

        static std::unique_ptr<ErrorMessage> fromJson(const nlohmann::json& json);

        const std::string& getCode() const { return m_code; }
        const std::string& getMessage() const { return m_message; }
        const nlohmann::json& getDetails() const { return m_details; }

        // Common error codes
        static constexpr const char* INVALID_SHOT = "INVALID_SHOT";
        static constexpr const char* INVALID_STATE = "INVALID_STATE";
        static constexpr const char* PROTOCOL_ERROR = "PROTOCOL_ERROR";
        static constexpr const char* INTERNAL_ERROR = "INTERNAL_ERROR";
        static constexpr const char* ALREADY_SHOT = "ALREADY_SHOT";
        static constexpr const char* NOT_YOUR_TURN = "NOT_YOUR_TURN";

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["code"] = m_code;
            payload["message"] = m_message;
            if (!m_details.empty())
            {
                payload["details"] = m_details;
            }
            return payload;
        }

        bool validatePayload() const override
        {
            return !m_code.empty() && !m_message.empty();
        }

    private:
        std::string m_code;
        std::string m_message;
        nlohmann::json m_details;
    };

    /**
     * @brief Session end message - graceful session termination
     */
    class SessionEndMessage : public BaseMessage
    {
    public:
        enum class Reason
        {
            Normal,
            Forfeit,
            Disconnect,
            Error
        };

        SessionEndMessage(Reason reason, std::string message = "", 
                         std::optional<int> winner = std::nullopt)
            : BaseMessage("session_end")
            , m_reason(reason)
            , m_message(std::move(message))
            , m_winner(winner)
        {
        }

        static std::unique_ptr<SessionEndMessage> fromJson(const nlohmann::json& json);

        Reason getReason() const { return m_reason; }
        const std::string& getMessage() const { return m_message; }
        std::optional<int> getWinner() const { return m_winner; }

        static std::string reasonToString(Reason reason)
        {
            switch (reason)
            {
            case Reason::Normal: return "normal";
            case Reason::Forfeit: return "forfeit";
            case Reason::Disconnect: return "disconnect";
            case Reason::Error: return "error";
            default: return "unknown";
            }
        }

        static Reason stringToReason(const std::string& str)
        {
            if (str == "normal") return Reason::Normal;
            if (str == "forfeit") return Reason::Forfeit;
            if (str == "disconnect") return Reason::Disconnect;
            if (str == "error") return Reason::Error;
            throw std::invalid_argument("Invalid reason string: " + str);
        }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["reason"] = reasonToString(m_reason);
            if (!m_message.empty())
            {
                payload["message"] = m_message;
            }
            if (m_winner.has_value())
            {
                payload["winner"] = m_winner.value();
            }
            return payload;
        }

        bool validatePayload() const override
        {
            return true; // All fields are optional or have defaults
        }

    private:
        Reason m_reason;
        std::string m_message;
        std::optional<int> m_winner;
    };

    /**
     * @brief Forfeit message - player forfeiting the game
     */
    class ForfeitMessage : public BaseMessage
    {
    public:
        ForfeitMessage(int playerId, std::string reason = "")
            : BaseMessage("forfeit")
            , m_playerId(playerId)
            , m_reason(std::move(reason))
        {
        }

        static std::unique_ptr<ForfeitMessage> fromJson(const nlohmann::json& json);

        int getPlayerId() const { return m_playerId; }
        const std::string& getReason() const { return m_reason; }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["player_id"] = m_playerId;
            if (!m_reason.empty())
            {
                payload["reason"] = m_reason;
            }
            return payload;
        }

        bool validatePayload() const override
        {
            return m_playerId == 0 || m_playerId == 1;
        }

    private:
        int m_playerId;
        std::string m_reason;
    };

    /**
     * @brief Chat message - player-to-player communication (reserved for future use)
     */
    class ChatMessage : public BaseMessage
    {
    public:
        ChatMessage(int playerId, std::string message)
            : BaseMessage("chat")
            , m_playerId(playerId)
            , m_message(std::move(message))
        {
        }

        static std::unique_ptr<ChatMessage> fromJson(const nlohmann::json& json);

        int getPlayerId() const { return m_playerId; }
        const std::string& getMessage() const { return m_message; }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["player_id"] = m_playerId;
            payload["message"] = m_message;
            return payload;
        }

        bool validatePayload() const override
        {
            return (m_playerId == 0 || m_playerId == 1) && 
                   !m_message.empty() && 
                   m_message.length() <= 500;
        }

    private:
        int m_playerId;
        std::string m_message;
    };

    /**
     * @brief Game state synchronization message
     */
    class GameStateMessage : public BaseMessage
    {
    public:
        enum class Status
        {
            Waiting,
            Playing,
            Finished
        };

        GameStateMessage(int currentPlayer, int playerId, Status status)
            : BaseMessage("game_state")
            , m_currentPlayer(currentPlayer)
            , m_playerId(playerId)
            , m_status(status)
        {
        }

        static std::unique_ptr<GameStateMessage> fromJson(const nlohmann::json& json);

        int getCurrentPlayer() const { return m_currentPlayer; }
        int getPlayerId() const { return m_playerId; }
        Status getStatus() const { return m_status; }

        static std::string statusToString(Status status)
        {
            switch (status)
            {
            case Status::Waiting: return "waiting";
            case Status::Playing: return "playing";
            case Status::Finished: return "finished";
            default: return "unknown";
            }
        }

        static Status stringToStatus(const std::string& str)
        {
            if (str == "waiting") return Status::Waiting;
            if (str == "playing") return Status::Playing;
            if (str == "finished") return Status::Finished;
            throw std::invalid_argument("Invalid status string: " + str);
        }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["current_player"] = m_currentPlayer;
            payload["player_id"] = m_playerId;
            payload["game_status"] = statusToString(m_status);
            return payload;
        }

        bool validatePayload() const override
        {
            return (m_currentPlayer == 0 || m_currentPlayer == 1) &&
                   (m_playerId == 0 || m_playerId == 1);
        }

    private:
        int m_currentPlayer;
        int m_playerId;
        Status m_status;
    };

    /**
     * @brief Handshake message - initial connection establishment
     */
    class HandshakeMessage : public BaseMessage
    {
    public:
        HandshakeMessage(std::string clientVersion, std::string playerName = "")
            : BaseMessage("handshake")
            , m_clientVersion(std::move(clientVersion))
            , m_playerName(std::move(playerName))
        {
        }

        static std::unique_ptr<HandshakeMessage> fromJson(const nlohmann::json& json);

        const std::string& getClientVersion() const { return m_clientVersion; }
        const std::string& getPlayerName() const { return m_playerName; }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["client_version"] = m_clientVersion;
            if (!m_playerName.empty())
            {
                payload["player_name"] = m_playerName;
            }
            return payload;
        }

        bool validatePayload() const override
        {
            return !m_clientVersion.empty();
        }

    private:
        std::string m_clientVersion;
        std::string m_playerName;
    };

    /**
     * @brief Handshake acknowledgment message - server response to handshake
     */
    class HandshakeAckMessage : public BaseMessage
    {
    public:
        HandshakeAckMessage(bool accepted, int playerId, std::string sessionId)
            : BaseMessage("handshake_ack")
            , m_accepted(accepted)
            , m_playerId(playerId)
            , m_sessionId(std::move(sessionId))
        {
        }

        static std::unique_ptr<HandshakeAckMessage> fromJson(const nlohmann::json& json);

        bool isAccepted() const { return m_accepted; }
        int getPlayerId() const { return m_playerId; }
        const std::string& getSessionId() const { return m_sessionId; }

    protected:
        nlohmann::json getPayload() const override
        {
            nlohmann::json payload;
            payload["accepted"] = m_accepted;
            payload["player_id"] = m_playerId;
            payload["session_id"] = m_sessionId;
            return payload;
        }

        bool validatePayload() const override
        {
            return !m_sessionId.empty() && (m_playerId == 0 || m_playerId == 1);
        }

    private:
        bool m_accepted;
        int m_playerId;
        std::string m_sessionId;
    };

} // namespace SeaBattle::Network
