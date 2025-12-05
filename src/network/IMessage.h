#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace SeaBattle::Network
{
    /**
     * @brief Base interface for all protocol messages
     * 
     * Provides common functionality for message serialization, deserialization,
     * and validation. All network messages must implement this interface.
     */
    class IMessage
    {
    public:
        virtual ~IMessage() = default;

        /**
         * @brief Get the message type identifier
         * @return Message type string (e.g., "shot", "response", "error")
         */
        virtual std::string getType() const = 0;

        /**
         * @brief Get the protocol version
         * @return Protocol version string (e.g., "1.0")
         */
        virtual std::string getVersion() const = 0;

        /**
         * @brief Get the message timestamp
         * @return Unix timestamp in milliseconds
         */
        virtual int64_t getTimestamp() const = 0;

        /**
         * @brief Serialize the message to JSON
         * @return JSON object representing the complete message
         */
        virtual nlohmann::json toJson() const = 0;

        /**
         * @brief Validate the message content
         * @return true if message is valid, false otherwise
         */
        virtual bool validate() const = 0;

        /**
         * @brief Get a string representation of the message (for debugging)
         * @return Human-readable message representation
         */
        virtual std::string toString() const
        {
            return toJson().dump(2);
        }
    };

    /**
     * @brief Abstract base class for protocol messages with common fields
     * 
     * Implements common message envelope structure with type, version, and timestamp.
     * Derived classes only need to implement payload-specific methods.
     */
    class BaseMessage : public IMessage
    {
    public:
        BaseMessage(std::string type, std::string version = ProtocolConstants::PROTOCOL_VERSION)
            : m_type(std::move(type))
            , m_version(std::move(version))
            , m_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count())
        {
        }

        BaseMessage(std::string type, std::string version, int64_t timestamp)
            : m_type(std::move(type))
            , m_version(std::move(version))
            , m_timestamp(timestamp)
        {
        }

        std::string getType() const override { return m_type; }
        std::string getVersion() const override { return m_version; }
        int64_t getTimestamp() const override { return m_timestamp; }

        nlohmann::json toJson() const override
        {
            nlohmann::json j;
            j["type"] = m_type;
            j["version"] = m_version;
            j["timestamp"] = m_timestamp;
            j["payload"] = getPayload();
            return j;
        }

        bool validate() const override
        {
            return !m_type.empty() && !m_version.empty() && validatePayload();
        }

    protected:
        /**
         * @brief Get the message-specific payload
         * @return JSON object representing the payload
         */
        virtual nlohmann::json getPayload() const = 0;

        /**
         * @brief Validate the message-specific payload
         * @return true if payload is valid, false otherwise
         */
        virtual bool validatePayload() const = 0;

    private:
        std::string m_type;
        std::string m_version;
        int64_t m_timestamp;
    };

    /**
     * @brief Factory for creating messages from JSON
     */
    class MessageFactory
    {
    public:
        /**
         * @brief Create a message from JSON data
         * @param json JSON object containing message data
         * @return Unique pointer to the created message, or nullptr if creation failed
         */
        static std::unique_ptr<IMessage> createFromJson(const nlohmann::json& json);

        /**
         * @brief Parse JSON string and create a message
         * @param jsonString JSON string containing message data
         * @return Unique pointer to the created message, or nullptr if parsing/creation failed
         */
        static std::unique_ptr<IMessage> createFromString(const std::string& jsonString);
    };

} // namespace SeaBattle::Network
