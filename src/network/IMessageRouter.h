#pragma once

#include "IMessage.h"
#include "IMessageHandler.h"
#include <memory>
#include <string>
#include <map>
#include <functional>

namespace SeaBattle::Network
{
    /**
     * @brief Interface for message routing
     * 
     * Routes incoming messages to appropriate handlers based on message type.
     * Supports dynamic handler registration for extensibility.
     */
    class IMessageRouter
    {
    public:
        virtual ~IMessageRouter() = default;

        /**
         * @brief Register a handler for a specific message type
         * @param handler The handler to register
         * @return true if registration successful, false if type already registered
         */
        virtual bool registerHandler(std::unique_ptr<IMessageHandler> handler) = 0;

        /**
         * @brief Unregister a handler for a specific message type
         * @param messageType The message type to unregister
         * @return true if unregistration successful, false if type not found
         */
        virtual bool unregisterHandler(const std::string& messageType) = 0;

        /**
         * @brief Route a message to the appropriate handler
         * @param message The message to route
         * @return Response message from handler, or nullptr if no response
         * @throws std::runtime_error if no handler found for message type
         */
        virtual std::unique_ptr<IMessage> route(const IMessage& message) = 0;

        /**
         * @brief Check if a handler is registered for a message type
         * @param messageType The message type to check
         * @return true if handler is registered
         */
        virtual bool hasHandler(const std::string& messageType) const = 0;

        /**
         * @brief Validate protocol version compatibility
         * @param version The protocol version to check
         * @return true if version is supported
         */
        virtual bool isVersionSupported(const std::string& version) const = 0;
    };

    /**
     * @brief Default implementation of message router
     */
    class MessageRouter : public IMessageRouter
    {
    public:
        MessageRouter() = default;

        bool registerHandler(std::unique_ptr<IMessageHandler> handler) override
        {
            if (!handler)
            {
                return false;
            }

            std::string messageType = handler->getMessageType();
            if (m_handlers.find(messageType) != m_handlers.end())
            {
                return false; // Handler already registered
            }

            m_handlers[messageType] = std::move(handler);
            return true;
        }

        bool unregisterHandler(const std::string& messageType) override
        {
            auto it = m_handlers.find(messageType);
            if (it == m_handlers.end())
            {
                return false;
            }

            m_handlers.erase(it);
            return true;
        }

        std::unique_ptr<IMessage> route(const IMessage& message) override
        {
            // Validate protocol version
            if (!isVersionSupported(message.getVersion()))
            {
                throw std::runtime_error("Unsupported protocol version: " + message.getVersion());
            }

            // Validate message
            if (!message.validate())
            {
                throw std::runtime_error("Invalid message: " + message.getType());
            }

            // Find and invoke handler
            auto it = m_handlers.find(message.getType());
            if (it == m_handlers.end())
            {
                throw std::runtime_error("No handler registered for message type: " + message.getType());
            }

            return it->second->handle(message);
        }

        bool hasHandler(const std::string& messageType) const override
        {
            return m_handlers.find(messageType) != m_handlers.end();
        }

        bool isVersionSupported(const std::string& version) const override
        {
            // Currently only support version 1.0
            // Future versions should be added here
            return version == "1.0";
        }

        /**
         * @brief Get the number of registered handlers
         * @return Number of handlers
         */
        size_t getHandlerCount() const
        {
            return m_handlers.size();
        }

        /**
         * @brief Clear all registered handlers
         */
        void clearHandlers()
        {
            m_handlers.clear();
        }

    private:
        std::map<std::string, std::unique_ptr<IMessageHandler>> m_handlers;
    };

} // namespace SeaBattle::Network
