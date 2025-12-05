#pragma once

#include "IMessage.h"
#include <memory>
#include <string>

namespace SeaBattle::Network
{
    /**
     * @brief Base interface for message handlers
     * 
     * Implements the Command pattern for processing different message types.
     * Each handler is responsible for processing a specific message type.
     */
    class IMessageHandler
    {
    public:
        virtual ~IMessageHandler() = default;

        /**
         * @brief Get the message type this handler processes
         * @return Message type string (e.g., "shot", "response")
         */
        virtual std::string getMessageType() const = 0;

        /**
         * @brief Process a message and generate a response
         * @param message The message to process
         * @return Response message, or nullptr if no response needed
         */
        virtual std::unique_ptr<IMessage> handle(const IMessage& message) = 0;

        /**
         * @brief Check if this handler can process the given message
         * @param message The message to check
         * @return true if this handler can process the message
         */
        virtual bool canHandle(const IMessage& message) const
        {
            return message.getType() == getMessageType();
        }
    };

    /**
     * @brief Handler for shot messages
     */
    class IShotHandler : public IMessageHandler
    {
    public:
        std::string getMessageType() const override { return "shot"; }
        
        /**
         * @brief Process a shot message
         * @param message The shot message
         * @return ShotResponseMessage indicating the result
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

    /**
     * @brief Handler for shot response messages
     */
    class IResponseHandler : public IMessageHandler
    {
    public:
        std::string getMessageType() const override { return "shot_response"; }
        
        /**
         * @brief Process a shot response message
         * @param message The shot response message
         * @return nullptr (responses typically don't generate further messages)
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

    /**
     * @brief Handler for error messages
     */
    class IErrorHandler : public IMessageHandler
    {
    public:
        std::string getMessageType() const override { return "error"; }
        
        /**
         * @brief Process an error message
         * @param message The error message
         * @return nullptr (error handlers typically don't generate responses)
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

    /**
     * @brief Handler for session control messages (end, forfeit)
     */
    class ISessionHandler : public IMessageHandler
    {
    public:
        /**
         * @brief Process a session control message
         * @param message The session control message
         * @return Response message if needed
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

    /**
     * @brief Handler for session end messages
     */
    class ISessionEndHandler : public ISessionHandler
    {
    public:
        std::string getMessageType() const override { return "session_end"; }
    };

    /**
     * @brief Handler for forfeit messages
     */
    class IForfeitHandler : public ISessionHandler
    {
    public:
        std::string getMessageType() const override { return "forfeit"; }
    };

    /**
     * @brief Handler for chat messages (reserved for future use)
     */
    class IChatHandler : public IMessageHandler
    {
    public:
        std::string getMessageType() const override { return "chat"; }
        
        /**
         * @brief Process a chat message
         * @param message The chat message
         * @return nullptr or broadcast message
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

    /**
     * @brief Handler for game state messages
     */
    class IGameStateHandler : public IMessageHandler
    {
    public:
        std::string getMessageType() const override { return "game_state"; }
        
        /**
         * @brief Process a game state update message
         * @param message The game state message
         * @return nullptr (state updates typically don't generate responses)
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

    /**
     * @brief Handler for handshake messages
     */
    class IHandshakeHandler : public IMessageHandler
    {
    public:
        std::string getMessageType() const override { return "handshake"; }
        
        /**
         * @brief Process a handshake message
         * @param message The handshake message
         * @return HandshakeAckMessage response
         */
        std::unique_ptr<IMessage> handle(const IMessage& message) override = 0;
    };

} // namespace SeaBattle::Network
