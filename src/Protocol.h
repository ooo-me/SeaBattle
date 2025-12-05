#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace SeaBattle::Network
{
    // Message types for client-server communication
    enum class MessageType : uint8_t
    {
        // Connection messages
        Connect = 0,
        ConnectAck = 1,
        Disconnect = 2,
        
        // Game state messages
        GameStart = 10,
        GameOver = 11,
        PlayerTurn = 12,
        
        // Game action messages
        ShootRequest = 20,
        ShootResponse = 21,
        
        // Status messages
        Error = 30,
        Ping = 40,
        Pong = 41
    };

    // Result codes for responses
    enum class ResultCode : uint8_t
    {
        Success = 0,
        InvalidMove = 1,
        NotYourTurn = 2,
        GameNotStarted = 3,
        ServerError = 4,
        UnknownError = 255
    };

    // Message header structure
    struct MessageHeader
    {
        MessageType type;
        uint32_t payloadSize;
        
        static constexpr size_t SIZE = sizeof(MessageType) + sizeof(uint32_t);
        
        std::vector<uint8_t> serialize() const
        {
            std::vector<uint8_t> buffer(SIZE);
            buffer[0] = static_cast<uint8_t>(type);
            
            // Serialize payload size (big-endian)
            buffer[1] = (payloadSize >> 24) & 0xFF;
            buffer[2] = (payloadSize >> 16) & 0xFF;
            buffer[3] = (payloadSize >> 8) & 0xFF;
            buffer[4] = payloadSize & 0xFF;
            
            return buffer;
        }
        
        static MessageHeader deserialize(const std::vector<uint8_t>& buffer)
        {
            if (buffer.size() < SIZE)
            {
                throw std::runtime_error("Buffer too small for MessageHeader");
            }
            
            MessageHeader header;
            header.type = static_cast<MessageType>(buffer[0]);
            
            // Deserialize payload size (big-endian)
            header.payloadSize = (static_cast<uint32_t>(buffer[1]) << 24) |
                                (static_cast<uint32_t>(buffer[2]) << 16) |
                                (static_cast<uint32_t>(buffer[3]) << 8) |
                                static_cast<uint32_t>(buffer[4]);
            
            return header;
        }
    };

    // Base message class
    class Message
    {
    public:
        explicit Message(MessageType type) : type_(type) {}
        virtual ~Message() = default;
        
        MessageType getType() const { return type_; }
        
        virtual std::vector<uint8_t> serializePayload() const = 0;
        virtual void deserializePayload(const std::vector<uint8_t>& payload) = 0;
        
        std::vector<uint8_t> serialize() const
        {
            auto payload = serializePayload();
            MessageHeader header{type_, static_cast<uint32_t>(payload.size())};
            auto headerBytes = header.serialize();
            
            std::vector<uint8_t> result;
            result.reserve(headerBytes.size() + payload.size());
            result.insert(result.end(), headerBytes.begin(), headerBytes.end());
            result.insert(result.end(), payload.begin(), payload.end());
            
            return result;
        }
        
    private:
        MessageType type_;
    };

    // Connect message
    class ConnectMessage : public Message
    {
    public:
        ConnectMessage() : Message(MessageType::Connect) {}
        explicit ConnectMessage(std::string playerName) 
            : Message(MessageType::Connect), playerName_(std::move(playerName)) {}
        
        const std::string& getPlayerName() const { return playerName_; }
        
        std::vector<uint8_t> serializePayload() const override
        {
            std::vector<uint8_t> payload(playerName_.begin(), playerName_.end());
            return payload;
        }
        
        void deserializePayload(const std::vector<uint8_t>& payload) override
        {
            playerName_ = std::string(payload.begin(), payload.end());
        }
        
    private:
        std::string playerName_;
    };

    // Shoot request message
    class ShootRequestMessage : public Message
    {
    public:
        ShootRequestMessage() : Message(MessageType::ShootRequest), row_(0), col_(0) {}
        ShootRequestMessage(int row, int col) 
            : Message(MessageType::ShootRequest), row_(row), col_(col) {}
        
        int getRow() const { return row_; }
        int getCol() const { return col_; }
        
        std::vector<uint8_t> serializePayload() const override
        {
            std::vector<uint8_t> payload(2);
            payload[0] = static_cast<uint8_t>(row_);
            payload[1] = static_cast<uint8_t>(col_);
            return payload;
        }
        
        void deserializePayload(const std::vector<uint8_t>& payload) override
        {
            if (payload.size() < 2)
            {
                throw std::runtime_error("Invalid ShootRequest payload");
            }
            row_ = static_cast<int>(payload[0]);
            col_ = static_cast<int>(payload[1]);
        }
        
    private:
        int row_;
        int col_;
    };

    // Shoot response message
    class ShootResponseMessage : public Message
    {
    public:
        ShootResponseMessage() 
            : Message(MessageType::ShootResponse), 
              result_(ResultCode::Success), hit_(false) {}
        
        ShootResponseMessage(ResultCode result, bool hit) 
            : Message(MessageType::ShootResponse), 
              result_(result), hit_(hit) {}
        
        ResultCode getResult() const { return result_; }
        bool isHit() const { return hit_; }
        
        std::vector<uint8_t> serializePayload() const override
        {
            std::vector<uint8_t> payload(2);
            payload[0] = static_cast<uint8_t>(result_);
            payload[1] = hit_ ? 1 : 0;
            return payload;
        }
        
        void deserializePayload(const std::vector<uint8_t>& payload) override
        {
            if (payload.size() < 2)
            {
                throw std::runtime_error("Invalid ShootResponse payload");
            }
            result_ = static_cast<ResultCode>(payload[0]);
            hit_ = payload[1] != 0;
        }
        
    private:
        ResultCode result_;
        bool hit_;
    };

    // Error message
    class ErrorMessage : public Message
    {
    public:
        ErrorMessage() : Message(MessageType::Error) {}
        explicit ErrorMessage(std::string errorText) 
            : Message(MessageType::Error), errorText_(std::move(errorText)) {}
        
        const std::string& getErrorText() const { return errorText_; }
        
        std::vector<uint8_t> serializePayload() const override
        {
            std::vector<uint8_t> payload(errorText_.begin(), errorText_.end());
            return payload;
        }
        
        void deserializePayload(const std::vector<uint8_t>& payload) override
        {
            errorText_ = std::string(payload.begin(), payload.end());
        }
        
    private:
        std::string errorText_;
    };

    // Simple ping/pong messages (no payload)
    class PingMessage : public Message
    {
    public:
        PingMessage() : Message(MessageType::Ping) {}
        
        std::vector<uint8_t> serializePayload() const override { return {}; }
        void deserializePayload(const std::vector<uint8_t>&) override {}
    };

    class PongMessage : public Message
    {
    public:
        PongMessage() : Message(MessageType::Pong) {}
        
        std::vector<uint8_t> serializePayload() const override { return {}; }
        void deserializePayload(const std::vector<uint8_t>&) override {}
    };

} // namespace SeaBattle::Network
