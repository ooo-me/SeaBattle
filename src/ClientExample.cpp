// Example/Test program demonstrating NetworkClient usage
// This file is not part of the main application, but can be compiled separately for testing

#include "NetworkClient.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace SeaBattle::Network;

void printStatus(ConnectionStatus status, const std::string& message)
{
    std::cout << "[STATUS] " << connectionStatusToString(status) 
              << ": " << message << std::endl;
}

void printMessage(std::unique_ptr<Message> message)
{
    std::cout << "[MESSAGE] Received message of type: " 
              << static_cast<int>(message->getType()) << std::endl;
    
    switch (message->getType())
    {
        case MessageType::ShootResponse:
        {
            auto* response = dynamic_cast<ShootResponseMessage*>(message.get());
            if (response)
            {
                std::cout << "  Result: " << static_cast<int>(response->getResult())
                         << ", Hit: " << (response->isHit() ? "Yes" : "No") << std::endl;
            }
            break;
        }
        case MessageType::Error:
        {
            auto* error = dynamic_cast<ErrorMessage*>(message.get());
            if (error)
            {
                std::cout << "  Error: " << error->getErrorText() << std::endl;
            }
            break;
        }
        case MessageType::Pong:
        {
            std::cout << "  Pong received!" << std::endl;
            break;
        }
        default:
            break;
    }
}

void printSendResult(bool success, const std::string& error)
{
    if (success)
    {
        std::cout << "[SEND] Message sent successfully" << std::endl;
    }
    else
    {
        std::cout << "[SEND] Failed to send message: " << error << std::endl;
    }
}

int main()
{
    std::cout << "SeaBattle Network Client Example" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Create client
    auto client = std::make_shared<NetworkClient>();
    
    // Set callbacks
    client->setConnectionStatusCallback(printStatus);
    client->setMessageReceivedCallback(printMessage);
    client->setSendCompleteCallback(printSendResult);
    
    // Start client in separate thread
    std::thread clientThread([client]() {
        client->run();
    });
    
    // Example 1: Connect to server (will fail without actual server)
    std::cout << "\n[TEST] Attempting to connect to localhost:8080..." << std::endl;
    client->connectAsync("localhost", 8080, std::chrono::seconds(5));
    
    // Wait a bit for connection attempt
    std::this_thread::sleep_for(std::chrono::seconds(6));
    
    // Check status
    std::cout << "\n[TEST] Current status: " 
              << connectionStatusToString(client->getStatus()) << std::endl;
    
    // Example 2: Try to send a message (will fail if not connected)
    if (client->isConnected())
    {
        std::cout << "\n[TEST] Sending connect message..." << std::endl;
        client->sendMessage(std::make_unique<ConnectMessage>("TestPlayer"));
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "\n[TEST] Sending shoot request..." << std::endl;
        client->sendMessage(std::make_unique<ShootRequestMessage>(5, 5));
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "\n[TEST] Sending ping..." << std::endl;
        client->sendMessage(std::make_unique<PingMessage>());
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    else
    {
        std::cout << "\n[TEST] Not connected, skipping message send tests" << std::endl;
    }
    
    // Disconnect
    std::cout << "\n[TEST] Disconnecting..." << std::endl;
    client->disconnect();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Stop client
    client->stop();
    clientThread.join();
    
    std::cout << "\n[TEST] Test completed" << std::endl;
    
    return 0;
}
