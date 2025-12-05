# SeaBattle Network Protocol Usage Guide

## Overview

This guide provides practical examples of how to use the SeaBattle network protocol and architecture for implementing multiplayer functionality.

## Basic Usage Examples

### 1. Creating and Sending Messages

#### Shot Message Example

```cpp
#include "network/Messages.h"

// Create a shot message
auto shotMessage = std::make_unique<SeaBattle::Network::ShotMessage>(5, 3);

// Serialize to JSON
nlohmann::json json = shotMessage->toJson();
std::string jsonString = json.dump();

// Send over network
networkNode->sendMessage(*shotMessage);
```

#### Shot Response Example

```cpp
// Create a shot response
using Result = SeaBattle::Network::ShotResponseMessage::Result;
auto response = std::make_unique<SeaBattle::Network::ShotResponseMessage>(
    5, 3, Result::Hit, false);

// If this shot ends the game
auto gameOverResponse = std::make_unique<SeaBattle::Network::ShotResponseMessage>(
    9, 9, Result::Destroyed, true, 0); // Player 0 wins

networkNode->sendMessage(*response);
```

#### Error Message Example

```cpp
// Create an error message
using ErrorMessage = SeaBattle::Network::ErrorMessage;
auto errorMsg = std::make_unique<ErrorMessage>(
    ErrorMessage::INVALID_SHOT,
    "Shot coordinates out of bounds",
    nlohmann::json{{"row", 15}, {"col", 3}}
);

networkNode->sendMessage(*errorMsg);
```

### 2. Parsing Received Messages

```cpp
#include "network/MessageFactory.h"

// Receive JSON string from network
std::string jsonString = receiveFromNetwork();

// Parse into message object
auto message = SeaBattle::Network::MessageFactory::createFromString(jsonString);

if (!message) {
    // Parsing failed
    handleParsingError();
    return;
}

// Check message type and process
if (message->getType() == "shot") {
    auto* shotMsg = dynamic_cast<SeaBattle::Network::ShotMessage*>(message.get());
    if (shotMsg) {
        processShot(shotMsg->getRow(), shotMsg->getCol());
    }
}
else if (message->getType() == "shot_response") {
    auto* responseMsg = dynamic_cast<SeaBattle::Network::ShotResponseMessage*>(message.get());
    if (responseMsg) {
        processShotResponse(responseMsg);
    }
}
```

### 3. Setting Up Message Handlers

#### Implementing a Shot Handler

```cpp
#include "network/IMessageHandler.h"
#include "network/Messages.h"

class ShotHandlerImpl : public SeaBattle::Network::IShotHandler
{
public:
    ShotHandlerImpl(SeaBattle::GameModel* gameModel)
        : m_gameModel(gameModel)
    {}

    std::unique_ptr<SeaBattle::Network::IMessage> handle(
        const SeaBattle::Network::IMessage& message) override
    {
        // Cast to specific message type
        const auto* shotMsg = dynamic_cast<const SeaBattle::Network::ShotMessage*>(&message);
        if (!shotMsg) {
            return std::make_unique<SeaBattle::Network::ErrorMessage>(
                SeaBattle::Network::ErrorMessage::PROTOCOL_ERROR,
                "Invalid shot message");
        }

        int row = shotMsg->getRow();
        int col = shotMsg->getCol();

        // Validate coordinates
        if (row < 0 || row >= 10 || col < 0 || col >= 10) {
            return std::make_unique<SeaBattle::Network::ErrorMessage>(
                SeaBattle::Network::ErrorMessage::INVALID_SHOT,
                "Coordinates out of bounds",
                nlohmann::json{{"row", row}, {"col", col}});
        }

        // Process shot in game model
        bool hit = m_gameModel->shoot(row, col);
        
        // Determine result
        auto cellState = m_gameModel->getEnemyCellState(
            m_gameModel->getCurrentPlayer(), row, col);
        
        SeaBattle::Network::ShotResponseMessage::Result result;
        if (cellState == SeaBattle::CellState::Miss) {
            result = SeaBattle::Network::ShotResponseMessage::Result::Miss;
        }
        else if (cellState == SeaBattle::CellState::Hit) {
            result = SeaBattle::Network::ShotResponseMessage::Result::Hit;
        }
        else {
            result = SeaBattle::Network::ShotResponseMessage::Result::Destroyed;
        }

        // Check for game over
        bool gameOver = (m_gameModel->getGameState() == SeaBattle::GameState::GameOver);
        std::optional<int> winner;
        if (gameOver) {
            winner = m_gameModel->getWinner();
        }

        // Return response message
        return std::make_unique<SeaBattle::Network::ShotResponseMessage>(
            row, col, result, gameOver, winner);
    }

private:
    SeaBattle::GameModel* m_gameModel;
};
```

#### Implementing an Error Handler

```cpp
class ErrorHandlerImpl : public SeaBattle::Network::IErrorHandler
{
public:
    std::unique_ptr<SeaBattle::Network::IMessage> handle(
        const SeaBattle::Network::IMessage& message) override
    {
        const auto* errorMsg = dynamic_cast<const SeaBattle::Network::ErrorMessage*>(&message);
        if (!errorMsg) {
            return nullptr;
        }

        // Log error
        std::cerr << "Error received: " << errorMsg->getCode() 
                  << " - " << errorMsg->getMessage() << std::endl;

        // Display error to user
        displayErrorToUser(errorMsg->getMessage());

        // No response needed
        return nullptr;
    }

private:
    void displayErrorToUser(const std::string& message) {
        // Show error dialog or notification
    }
};
```

### 4. Setting Up Message Router

```cpp
#include "network/IMessageRouter.h"

// Create router
auto router = std::make_unique<SeaBattle::Network::MessageRouter>();

// Register handlers
router->registerHandler(std::make_unique<ShotHandlerImpl>(gameModel));
router->registerHandler(std::make_unique<ErrorHandlerImpl>());
router->registerHandler(std::make_unique<SessionEndHandlerImpl>());
router->registerHandler(std::make_unique<ForfeitHandlerImpl>());

// Route incoming message
auto message = receiveMessage();
try {
    auto response = router->route(*message);
    if (response) {
        // Send response back
        networkNode->sendMessage(*response);
    }
} catch (const std::exception& e) {
    // Handle routing error
    std::cerr << "Routing error: " << e.what() << std::endl;
}
```

## Client Implementation Example

### Basic Client Setup

```cpp
#include "network/INetworkNode.h"
#include "network/Messages.h"
#include <QTcpSocket>

class NetworkClient : public QObject, public SeaBattle::Network::IClient
{
    Q_OBJECT

public:
    NetworkClient(QObject* parent = nullptr)
        : QObject(parent)
        , m_socket(new QTcpSocket(this))
        , m_state(SeaBattle::Network::ConnectionState::Disconnected)
    {
        connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
        connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                this, &NetworkClient::onError);
    }

    bool connect(const std::string& host, int port) override
    {
        m_serverAddress = host;
        m_serverPort = port;
        m_state = SeaBattle::Network::ConnectionState::Connecting;
        
        m_socket->connectToHost(QString::fromStdString(host), port);
        return true;
    }

    bool sendMessage(const SeaBattle::Network::IMessage& message) override
    {
        if (!isConnected()) {
            return false;
        }

        // Serialize message
        std::string jsonString = message.toJson().dump();
        
        // Add message delimiter (newline)
        jsonString += "\n";
        
        // Send
        qint64 written = m_socket->write(jsonString.c_str(), jsonString.length());
        m_socket->flush();
        
        return written > 0;
    }

    SeaBattle::Network::ConnectionState getState() const override
    {
        return m_state;
    }

    void setMessageCallback(MessageCallback callback) override
    {
        m_messageCallback = callback;
    }

    void setErrorCallback(ErrorCallback callback) override
    {
        m_errorCallback = callback;
    }

    void setStateCallback(StateCallback callback) override
    {
        m_stateCallback = callback;
    }

    void close() override
    {
        if (m_socket->state() == QTcpSocket::ConnectedState) {
            m_socket->disconnectFromHost();
        }
        m_state = SeaBattle::Network::ConnectionState::Disconnected;
    }

    std::string getServerAddress() const override { return m_serverAddress; }
    int getServerPort() const override { return m_serverPort; }

private slots:
    void onConnected()
    {
        m_state = SeaBattle::Network::ConnectionState::Connected;
        if (m_stateCallback) {
            m_stateCallback(m_state);
        }
        
        // Send handshake
        auto handshake = std::make_unique<SeaBattle::Network::HandshakeMessage>(
            "1.0.0", "Player");
        sendMessage(*handshake);
    }

    void onDisconnected()
    {
        m_state = SeaBattle::Network::ConnectionState::Disconnected;
        if (m_stateCallback) {
            m_stateCallback(m_state);
        }
    }

    void onReadyRead()
    {
        // Read data
        QByteArray data = m_socket->readAll();
        m_buffer.append(data.toStdString());
        
        // Process complete messages (delimited by newlines)
        size_t pos;
        while ((pos = m_buffer.find('\n')) != std::string::npos) {
            std::string messageStr = m_buffer.substr(0, pos);
            m_buffer.erase(0, pos + 1);
            
            // Parse message
            auto message = SeaBattle::Network::MessageFactory::createFromString(messageStr);
            if (message && m_messageCallback) {
                m_messageCallback(std::move(message));
            }
        }
    }

    void onError(QAbstractSocket::SocketError error)
    {
        m_state = SeaBattle::Network::ConnectionState::Error;
        if (m_errorCallback) {
            m_errorCallback(m_socket->errorString().toStdString());
        }
    }

private:
    QTcpSocket* m_socket;
    SeaBattle::Network::ConnectionState m_state;
    std::string m_serverAddress;
    int m_serverPort;
    std::string m_buffer;
    
    MessageCallback m_messageCallback;
    ErrorCallback m_errorCallback;
    StateCallback m_stateCallback;
};
```

### Using the Client

```cpp
// Create client
auto client = std::make_unique<NetworkClient>();

// Set up callbacks
client->setMessageCallback([](std::unique_ptr<SeaBattle::Network::IMessage> message) {
    std::cout << "Received message: " << message->getType() << std::endl;
    
    if (message->getType() == "shot_response") {
        auto* response = dynamic_cast<SeaBattle::Network::ShotResponseMessage*>(message.get());
        if (response) {
            processShotResponse(response);
        }
    }
});

client->setErrorCallback([](const std::string& error) {
    std::cerr << "Network error: " << error << std::endl;
});

client->setStateCallback([](SeaBattle::Network::ConnectionState state) {
    std::cout << "Connection state changed: " << static_cast<int>(state) << std::endl;
});

// Connect to server
if (client->connect("127.0.0.1", 7777)) {
    std::cout << "Connecting to server..." << std::endl;
}

// Send a shot
auto shot = std::make_unique<SeaBattle::Network::ShotMessage>(5, 3);
client->sendMessage(*shot);
```

## Server Implementation Example

### Basic Server Setup

```cpp
#include "network/INetworkNode.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <map>

class NetworkServer : public QObject, public SeaBattle::Network::IServer
{
    Q_OBJECT

public:
    NetworkServer(QObject* parent = nullptr)
        : QObject(parent)
        , m_server(new QTcpServer(this))
        , m_nextClientId(0)
    {
        connect(m_server, &QTcpServer::newConnection, 
                this, &NetworkServer::onNewConnection);
    }

    bool listen(int port, int maxConnections = 2) override
    {
        m_maxConnections = maxConnections;
        return m_server->listen(QHostAddress::Any, port);
    }

    void stop() override
    {
        // Disconnect all clients
        for (auto& [id, socket] : m_clients) {
            socket->disconnectFromHost();
        }
        m_clients.clear();
        
        // Stop listening
        m_server->close();
    }

    bool sendMessage(const SeaBattle::Network::IMessage& message) override
    {
        // Broadcast to all clients by default
        return broadcastMessage(message) > 0;
    }

    bool sendMessageToClient(int clientId, const SeaBattle::Network::IMessage& message) override
    {
        auto it = m_clients.find(clientId);
        if (it == m_clients.end()) {
            return false;
        }

        std::string jsonString = message.toJson().dump() + "\n";
        qint64 written = it->second->write(jsonString.c_str(), jsonString.length());
        it->second->flush();
        
        return written > 0;
    }

    int broadcastMessage(const SeaBattle::Network::IMessage& message) override
    {
        std::string jsonString = message.toJson().dump() + "\n";
        int count = 0;
        
        for (auto& [id, socket] : m_clients) {
            if (socket->state() == QTcpSocket::ConnectedState) {
                socket->write(jsonString.c_str(), jsonString.length());
                socket->flush();
                count++;
            }
        }
        
        return count;
    }

    int getClientCount() const override
    {
        return m_clients.size();
    }

    std::vector<int> getClientIds() const override
    {
        std::vector<int> ids;
        for (const auto& [id, socket] : m_clients) {
            ids.push_back(id);
        }
        return ids;
    }

    bool disconnectClient(int clientId) override
    {
        auto it = m_clients.find(clientId);
        if (it == m_clients.end()) {
            return false;
        }
        
        it->second->disconnectFromHost();
        return true;
    }

    int getListeningPort() const override
    {
        return m_server->isListening() ? m_server->serverPort() : -1;
    }

    bool isListening() const override
    {
        return m_server->isListening();
    }

    // INetworkNode interface implementations
    SeaBattle::Network::ConnectionState getState() const override
    {
        return m_server->isListening() ? 
            SeaBattle::Network::ConnectionState::Connected :
            SeaBattle::Network::ConnectionState::Disconnected;
    }

    void setMessageCallback(MessageCallback callback) override
    {
        m_messageCallback = callback;
    }

    void setErrorCallback(ErrorCallback callback) override
    {
        m_errorCallback = callback;
    }

    void setStateCallback(StateCallback callback) override
    {
        m_stateCallback = callback;
    }

    void setClientConnectedCallback(ClientConnectedCallback callback) override
    {
        m_clientConnectedCallback = callback;
    }

    void setClientDisconnectedCallback(ClientDisconnectedCallback callback) override
    {
        m_clientDisconnectedCallback = callback;
    }

    void close() override
    {
        stop();
    }

private slots:
    void onNewConnection()
    {
        while (m_server->hasPendingConnections()) {
            QTcpSocket* socket = m_server->nextPendingConnection();
            
            if (m_clients.size() >= m_maxConnections) {
                // Reject connection
                socket->disconnectFromHost();
                socket->deleteLater();
                continue;
            }
            
            int clientId = m_nextClientId++;
            m_clients[clientId] = socket;
            m_clientBuffers[clientId] = "";
            
            connect(socket, &QTcpSocket::readyRead, 
                    this, [this, clientId]() { onClientReadyRead(clientId); });
            connect(socket, &QTcpSocket::disconnected,
                    this, [this, clientId]() { onClientDisconnected(clientId); });
            
            if (m_clientConnectedCallback) {
                m_clientConnectedCallback(clientId);
            }
        }
    }

    void onClientReadyRead(int clientId)
    {
        auto it = m_clients.find(clientId);
        if (it == m_clients.end()) {
            return;
        }

        QByteArray data = it->second->readAll();
        m_clientBuffers[clientId].append(data.toStdString());
        
        // Process complete messages
        std::string& buffer = m_clientBuffers[clientId];
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string messageStr = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            
            auto message = SeaBattle::Network::MessageFactory::createFromString(messageStr);
            if (message && m_messageCallback) {
                m_messageCallback(std::move(message));
            }
        }
    }

    void onClientDisconnected(int clientId)
    {
        if (m_clientDisconnectedCallback) {
            m_clientDisconnectedCallback(clientId);
        }
        
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            it->second->deleteLater();
            m_clients.erase(it);
        }
        
        m_clientBuffers.erase(clientId);
    }

private:
    QTcpServer* m_server;
    std::map<int, QTcpSocket*> m_clients;
    std::map<int, std::string> m_clientBuffers;
    int m_nextClientId;
    int m_maxConnections;
    
    MessageCallback m_messageCallback;
    ErrorCallback m_errorCallback;
    StateCallback m_stateCallback;
    ClientConnectedCallback m_clientConnectedCallback;
    ClientDisconnectedCallback m_clientDisconnectedCallback;
};
```

### Using the Server

```cpp
// Create server
auto server = std::make_unique<NetworkServer>();

// Set up callbacks
server->setClientConnectedCallback([](int clientId) {
    std::cout << "Client connected: " << clientId << std::endl;
});

server->setClientDisconnectedCallback([](int clientId) {
    std::cout << "Client disconnected: " << clientId << std::endl;
});

server->setMessageCallback([&server](std::unique_ptr<SeaBattle::Network::IMessage> message) {
    std::cout << "Received: " << message->getType() << std::endl;
    
    // Route through message router
    auto response = router->route(*message);
    if (response) {
        // Broadcast response to all clients
        server->broadcastMessage(*response);
    }
});

// Start listening
if (server->listen(7777)) {
    std::cout << "Server listening on port 7777" << std::endl;
}
```

## Complete Game Flow Example

### Client Side Game Flow

```cpp
class MultiplayerGameController : public QObject
{
    Q_OBJECT

public:
    MultiplayerGameController(SeaBattle::GameModel* model)
        : m_gameModel(model)
        , m_client(new NetworkClient(this))
        , m_myPlayerId(-1)
    {
        setupCallbacks();
    }

    void connectToServer(const QString& host, int port)
    {
        m_client->connect(host.toStdString(), port);
    }

    void takeShot(int row, int col)
    {
        if (!m_gameModel->isValidShot(row, col)) {
            emit shotRejected("Invalid shot coordinates");
            return;
        }

        auto shotMsg = std::make_unique<SeaBattle::Network::ShotMessage>(row, col);
        if (m_client->sendMessage(*shotMsg)) {
            emit shotSent(row, col);
        }
    }

    void forfeitGame()
    {
        auto forfeitMsg = std::make_unique<SeaBattle::Network::ForfeitMessage>(
            m_myPlayerId, "Player forfeited");
        m_client->sendMessage(*forfeitMsg);
    }

signals:
    void connected();
    void disconnected();
    void gameStarted();
    void shotSent(int row, int col);
    void shotRejected(QString reason);
    void shotResult(int row, int col, QString result);
    void gameOver(int winner);
    void errorOccurred(QString error);
    void myTurn();
    void opponentTurn();

private:
    void setupCallbacks()
    {
        m_client->setMessageCallback([this](auto message) {
            handleMessage(std::move(message));
        });

        m_client->setStateCallback([this](auto state) {
            if (state == SeaBattle::Network::ConnectionState::Connected) {
                emit connected();
            }
            else if (state == SeaBattle::Network::ConnectionState::Disconnected) {
                emit disconnected();
            }
        });

        m_client->setErrorCallback([this](const std::string& error) {
            emit errorOccurred(QString::fromStdString(error));
        });
    }

    void handleMessage(std::unique_ptr<SeaBattle::Network::IMessage> message)
    {
        const std::string type = message->getType();

        if (type == "handshake_ack") {
            auto* ack = dynamic_cast<SeaBattle::Network::HandshakeAckMessage*>(message.get());
            if (ack && ack->isAccepted()) {
                m_myPlayerId = ack->getPlayerId();
            }
        }
        else if (type == "game_state") {
            auto* state = dynamic_cast<SeaBattle::Network::GameStateMessage*>(message.get());
            if (state) {
                if (state->getStatus() == SeaBattle::Network::GameStateMessage::Status::Playing) {
                    emit gameStarted();
                }
                
                if (state->getCurrentPlayer() == m_myPlayerId) {
                    emit myTurn();
                } else {
                    emit opponentTurn();
                }
            }
        }
        else if (type == "shot_response") {
            auto* response = dynamic_cast<SeaBattle::Network::ShotResponseMessage*>(message.get());
            if (response) {
                QString result = QString::fromStdString(
                    SeaBattle::Network::ShotResponseMessage::resultToString(response->getResult()));
                emit shotResult(response->getRow(), response->getCol(), result);
                
                if (response->isGameOver()) {
                    emit gameOver(response->getWinner().value_or(-1));
                }
            }
        }
        else if (type == "error") {
            auto* error = dynamic_cast<SeaBattle::Network::ErrorMessage*>(message.get());
            if (error) {
                emit errorOccurred(QString::fromStdString(error->getMessage()));
            }
        }
        else if (type == "session_end") {
            auto* end = dynamic_cast<SeaBattle::Network::SessionEndMessage*>(message.get());
            if (end && end->getWinner().has_value()) {
                emit gameOver(end->getWinner().value());
            }
            m_client->close();
        }
    }

    SeaBattle::GameModel* m_gameModel;
    NetworkClient* m_client;
    int m_myPlayerId;
};
```

## Testing Examples

### Unit Test for Message Serialization

```cpp
#include <gtest/gtest.h>
#include "network/Messages.h"

TEST(MessageTests, ShotMessageSerialization)
{
    // Create message
    SeaBattle::Network::ShotMessage msg(5, 3);
    
    // Serialize
    nlohmann::json json = msg.toJson();
    
    // Verify structure
    EXPECT_EQ(json["type"], "shot");
    EXPECT_EQ(json["version"], "1.0");
    EXPECT_EQ(json["payload"]["row"], 5);
    EXPECT_EQ(json["payload"]["col"], 3);
    
    // Verify validation
    EXPECT_TRUE(msg.validate());
}

TEST(MessageTests, ShotMessageValidation)
{
    // Valid message
    SeaBattle::Network::ShotMessage validMsg(5, 3);
    EXPECT_TRUE(validMsg.validate());
    
    // Invalid coordinates should be caught during construction
    // or validation
}

TEST(MessageTests, ShotResponseWithGameOver)
{
    using Result = SeaBattle::Network::ShotResponseMessage::Result;
    
    SeaBattle::Network::ShotResponseMessage msg(
        9, 9, Result::Destroyed, true, 0);
    
    EXPECT_TRUE(msg.isGameOver());
    EXPECT_EQ(msg.getWinner().value(), 0);
    EXPECT_TRUE(msg.validate());
}
```

### Integration Test for Message Flow

```cpp
TEST(IntegrationTests, CompleteShotFlow)
{
    // Setup
    auto gameModel = std::make_unique<SeaBattle::GameModel>();
    auto router = std::make_unique<SeaBattle::Network::MessageRouter>();
    router->registerHandler(std::make_unique<ShotHandlerImpl>(gameModel.get()));
    
    // Create shot message
    SeaBattle::Network::ShotMessage shot(5, 3);
    
    // Route message
    auto response = router->route(shot);
    
    // Verify response
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->getType(), "shot_response");
    
    auto* shotResponse = dynamic_cast<SeaBattle::Network::ShotResponseMessage*>(response.get());
    ASSERT_NE(shotResponse, nullptr);
    EXPECT_EQ(shotResponse->getRow(), 5);
    EXPECT_EQ(shotResponse->getCol(), 3);
}
```

## Conclusion

This guide provides practical examples for using the SeaBattle network protocol. For more details, refer to:
- `PROTOCOL.md` for protocol specification
- `ARCHITECTURE.md` for architectural design
- Header files in `src/network/` for API reference
