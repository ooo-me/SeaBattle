#pragma once

#include "IMessage.h"
#include "ProtocolConstants.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace SeaBattle::Network
{
    /**
     * @brief Connection state enumeration
     */
    enum class ConnectionState
    {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting,
        Error
    };

    /**
     * @brief Base interface for network nodes (client/server)
     * 
     * Provides common networking functionality for both client and server implementations.
     */
    class INetworkNode
    {
    public:
        using MessageCallback = std::function<void(std::unique_ptr<IMessage>)>;
        using ErrorCallback = std::function<void(const std::string&)>;
        using StateCallback = std::function<void(ConnectionState)>;

        virtual ~INetworkNode() = default;

        /**
         * @brief Send a message
         * @param message The message to send
         * @return true if message was queued for sending successfully
         */
        virtual bool sendMessage(const IMessage& message) = 0;

        /**
         * @brief Get the current connection state
         * @return Current connection state
         */
        virtual ConnectionState getState() const = 0;

        /**
         * @brief Set callback for received messages
         * @param callback Function to call when a message is received
         */
        virtual void setMessageCallback(MessageCallback callback) = 0;

        /**
         * @brief Set callback for errors
         * @param callback Function to call when an error occurs
         */
        virtual void setErrorCallback(ErrorCallback callback) = 0;

        /**
         * @brief Set callback for state changes
         * @param callback Function to call when connection state changes
         */
        virtual void setStateCallback(StateCallback callback) = 0;

        /**
         * @brief Close the connection
         */
        virtual void close() = 0;

        /**
         * @brief Check if node is connected
         * @return true if in Connected state
         */
        bool isConnected() const
        {
            return getState() == ConnectionState::Connected;
        }
    };

    /**
     * @brief Client interface for network communication
     * 
     * Represents a client that connects to a server.
     */
    class IClient : public INetworkNode
    {
    public:
        /**
         * @brief Connect to a server
         * @param host Server hostname or IP address
         * @param port Server port number
         * @return true if connection initiated successfully
         */
        virtual bool connect(const std::string& host, int port) = 0;

        /**
         * @brief Get the server address
         * @return Server address string
         */
        virtual std::string getServerAddress() const = 0;

        /**
         * @brief Get the server port
         * @return Server port number
         */
        virtual int getServerPort() const = 0;
    };

    /**
     * @brief Server interface for network communication
     * 
     * Represents a server that accepts client connections.
     */
    class IServer : public INetworkNode
    {
    public:
        using ClientConnectedCallback = std::function<void(int /*clientId*/)>;
        using ClientDisconnectedCallback = std::function<void(int /*clientId*/)>;

        /**
         * @brief Start listening for connections
         * @param port Port number to listen on
         * @param maxConnections Maximum number of concurrent connections (default for 1v1)
         * @return true if listening started successfully
         */
        virtual bool listen(int port, int maxConnections = ProtocolConstants::DEFAULT_MAX_CONNECTIONS) = 0;

        /**
         * @brief Stop listening and close all connections
         */
        virtual void stop() = 0;

        /**
         * @brief Send a message to a specific client
         * @param clientId The client ID to send to
         * @param message The message to send
         * @return true if message was queued for sending successfully
         */
        virtual bool sendMessageToClient(int clientId, const IMessage& message) = 0;

        /**
         * @brief Broadcast a message to all connected clients
         * @param message The message to broadcast
         * @return Number of clients the message was sent to
         */
        virtual int broadcastMessage(const IMessage& message) = 0;

        /**
         * @brief Get the number of connected clients
         * @return Number of connected clients
         */
        virtual int getClientCount() const = 0;

        /**
         * @brief Get list of connected client IDs
         * @return Vector of client IDs
         */
        virtual std::vector<int> getClientIds() const = 0;

        /**
         * @brief Disconnect a specific client
         * @param clientId The client ID to disconnect
         * @return true if client was disconnected
         */
        virtual bool disconnectClient(int clientId) = 0;

        /**
         * @brief Set callback for client connections
         * @param callback Function to call when a client connects
         */
        virtual void setClientConnectedCallback(ClientConnectedCallback callback) = 0;

        /**
         * @brief Set callback for client disconnections
         * @param callback Function to call when a client disconnects
         */
        virtual void setClientDisconnectedCallback(ClientDisconnectedCallback callback) = 0;

        /**
         * @brief Get the listening port
         * @return Port number, or -1 if not listening
         */
        virtual int getListeningPort() const = 0;

        /**
         * @brief Check if server is listening
         * @return true if server is accepting connections
         */
        virtual bool isListening() const = 0;
    };

    /**
     * @brief Connection manager for server-side client management
     * 
     * Manages multiple client connections and their states.
     */
    class IConnectionManager
    {
    public:
        virtual ~IConnectionManager() = default;

        /**
         * @brief Add a new client connection
         * @param clientId Unique client identifier
         * @return true if client was added successfully
         */
        virtual bool addClient(int clientId) = 0;

        /**
         * @brief Remove a client connection
         * @param clientId The client ID to remove
         * @return true if client was removed
         */
        virtual bool removeClient(int clientId) = 0;

        /**
         * @brief Check if a client is connected
         * @param clientId The client ID to check
         * @return true if client is connected
         */
        virtual bool isClientConnected(int clientId) const = 0;

        /**
         * @brief Get the number of connected clients
         * @return Number of connected clients
         */
        virtual int getClientCount() const = 0;

        /**
         * @brief Get list of all client IDs
         * @return Vector of client IDs
         */
        virtual std::vector<int> getClientIds() const = 0;

        /**
         * @brief Clear all client connections
         */
        virtual void clearAll() = 0;
    };

    /**
     * @brief Session manager for game session management
     * 
     * Manages game sessions and their associated data.
     */
    class ISessionManager
    {
    public:
        virtual ~ISessionManager() = default;

        /**
         * @brief Create a new game session
         * @param sessionId Unique session identifier
         * @param player1Id First player ID
         * @param player2Id Second player ID
         * @return true if session was created successfully
         */
        virtual bool createSession(const std::string& sessionId, int player1Id, int player2Id) = 0;

        /**
         * @brief Close a game session
         * @param sessionId The session ID to close
         * @return true if session was closed
         */
        virtual bool closeSession(const std::string& sessionId) = 0;

        /**
         * @brief Check if a session exists
         * @param sessionId The session ID to check
         * @return true if session exists
         */
        virtual bool sessionExists(const std::string& sessionId) const = 0;

        /**
         * @brief Get the session ID for a player
         * @param playerId The player ID to look up
         * @return Session ID, or empty string if not found
         */
        virtual std::string getSessionForPlayer(int playerId) const = 0;

        /**
         * @brief Get player IDs in a session
         * @param sessionId The session ID
         * @return Vector of player IDs in the session
         */
        virtual std::vector<int> getSessionPlayers(const std::string& sessionId) const = 0;
    };

    /**
     * @brief Message queue for outgoing messages
     * 
     * Buffers messages for sending and manages send order.
     */
    class IMessageQueue
    {
    public:
        virtual ~IMessageQueue() = default;

        /**
         * @brief Enqueue a message for sending
         * @param message The message to enqueue
         * @return true if message was enqueued successfully
         */
        virtual bool enqueue(std::unique_ptr<IMessage> message) = 0;

        /**
         * @brief Dequeue the next message
         * @return Next message, or nullptr if queue is empty
         */
        virtual std::unique_ptr<IMessage> dequeue() = 0;

        /**
         * @brief Check if queue is empty
         * @return true if no messages in queue
         */
        virtual bool isEmpty() const = 0;

        /**
         * @brief Get the number of messages in queue
         * @return Number of queued messages
         */
        virtual size_t size() const = 0;

        /**
         * @brief Clear all messages from queue
         */
        virtual void clear() = 0;
    };

} // namespace SeaBattle::Network
