/**
 * @file NetworkClientUsageExample.cpp
 * @brief Comprehensive example demonstrating NetworkClient and GameNetworkAdapter usage
 * 
 * This example shows how to:
 * 1. Use the low-level NetworkClient for direct message handling
 * 2. Use the high-level GameNetworkAdapter for game integration
 * 3. Handle connection statuses and errors
 * 4. Send and receive game messages
 */

#include "NetworkClient.h"
#include "GameNetworkAdapter.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace SeaBattle::Network;

// =============================================================================
// Example 1: Low-Level NetworkClient Usage
// =============================================================================

void example1_lowLevel()
{
    std::cout << "\n========================================\n";
    std::cout << "Example 1: Low-Level NetworkClient Usage\n";
    std::cout << "========================================\n\n";

    // Create client
    auto client = std::make_shared<NetworkClient>();

    // Set up connection status callback
    client->setConnectionStatusCallback(
        [](ConnectionStatus status, const std::string& message) {
            std::cout << "[CONNECTION] Status: " << std::setw(15) << std::left 
                     << connectionStatusToString(status) 
                     << " | " << message << std::endl;
        });

    // Set up message received callback
    client->setMessageReceivedCallback(
        [](std::unique_ptr<Message> message) {
            std::cout << "[MESSAGE] Received: ";
            
            switch (message->getType())
            {
                case MessageType::ShootResponse:
                {
                    auto* response = dynamic_cast<ShootResponseMessage*>(message.get());
                    if (response)
                    {
                        std::cout << "ShootResponse - Result: " 
                                 << static_cast<int>(response->getResult())
                                 << ", Hit: " << (response->isHit() ? "YES" : "NO");
                    }
                    break;
                }
                
                case MessageType::Error:
                {
                    auto* error = dynamic_cast<ErrorMessage*>(message.get());
                    if (error)
                    {
                        std::cout << "Error - " << error->getErrorText();
                    }
                    break;
                }
                
                case MessageType::Pong:
                    std::cout << "Pong received (server is alive)";
                    break;
                    
                default:
                    std::cout << "Type " << static_cast<int>(message->getType());
                    break;
            }
            
            std::cout << std::endl;
        });

    // Set up send complete callback
    client->setSendCompleteCallback(
        [](bool success, const std::string& error) {
            if (success)
            {
                std::cout << "[SEND] Message sent successfully" << std::endl;
            }
            else
            {
                std::cerr << "[SEND] Failed: " << error << std::endl;
            }
        });

    // Start client in separate thread
    std::thread clientThread([client]() {
        std::cout << "[THREAD] Network client thread started\n";
        client->run();
        std::cout << "[THREAD] Network client thread stopped\n";
    });

    // Connect to server with 5 second timeout
    std::cout << "[ACTION] Connecting to localhost:8080 (timeout: 5s)...\n";
    client->connectAsync("localhost", 8080, std::chrono::seconds(5));

    // Wait for connection attempt
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Check connection status
    std::cout << "\n[STATUS] Current connection status: " 
              << connectionStatusToString(client->getStatus()) << std::endl;

    // If connected, send some messages
    if (client->isConnected())
    {
        std::cout << "\n[ACTION] Sending messages...\n";
        
        // Send connect message
        client->sendMessage(std::make_unique<ConnectMessage>("PlayerOne"));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Send shoot request
        client->sendMessage(std::make_unique<ShootRequestMessage>(5, 5));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Send ping
        client->sendMessage(std::make_unique<PingMessage>());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    else
    {
        std::cout << "\n[INFO] Not connected - this is expected without a server\n";
    }

    // Disconnect
    std::cout << "\n[ACTION] Disconnecting...\n";
    client->disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop client
    client->stop();
    clientThread.join();

    std::cout << "\n[DONE] Example 1 completed\n";
}

// =============================================================================
// Example 2: High-Level GameNetworkAdapter Usage
// =============================================================================

void example2_highLevel()
{
    std::cout << "\n========================================\n";
    std::cout << "Example 2: High-Level GameNetworkAdapter\n";
    std::cout << "========================================\n\n";

    // Create game network adapter
    GameNetworkAdapter adapter;

    // Track connection ready state
    bool isReady = false;

    // Set up game action callback
    adapter.setGameActionCallback(
        [](int row, int col, bool hit) {
            if (row >= 0 && col >= 0)
            {
                std::cout << "[GAME] Shot at (" << row << "," << col << ") - "
                         << (hit ? "HIT!" : "MISS") << std::endl;
            }
            else
            {
                std::cout << "[GAME] Shoot response: " 
                         << (hit ? "HIT!" : "MISS") << std::endl;
            }
        });

    // Set up game state change callback
    adapter.setGameStateChangeCallback(
        [](GameState state) {
            std::cout << "[GAME] State changed to: " 
                     << static_cast<int>(state) << std::endl;
        });

    // Set up error callback
    adapter.setConnectionErrorCallback(
        [&isReady](const std::string& error) {
            std::cerr << "[ERROR] " << error << std::endl;
            isReady = false;
        });

    // Connect to server
    std::cout << "[ACTION] Connecting to localhost:8080 as 'TestPlayer'...\n";
    adapter.connect("localhost", 8080, "TestPlayer");

    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Check connection
    std::cout << "\n[STATUS] Connection status: " 
              << connectionStatusToString(adapter.getConnectionStatus()) << std::endl;
    std::cout << "[STATUS] Status message: " << adapter.getStatusMessage() << std::endl;

    if (adapter.isConnected())
    {
        isReady = true;
        std::cout << "\n[ACTION] Connected! Sending game actions...\n";

        // Send some game actions
        for (int i = 0; i < 3 && isReady; ++i)
        {
            std::cout << "[ACTION] Sending shoot action #" << (i + 1) << "...\n";
            adapter.sendShootAction(i * 2, i * 3);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Send ping
        std::cout << "[ACTION] Sending ping to check connectivity...\n";
        adapter.sendPing();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    else
    {
        std::cout << "\n[INFO] Not connected - this is expected without a server\n";
    }

    // Disconnect
    std::cout << "\n[ACTION] Disconnecting...\n";
    adapter.disconnect();

    std::cout << "\n[DONE] Example 2 completed\n";
}

// =============================================================================
// Example 3: Connection Status Demonstration
// =============================================================================

void example3_statusDemo()
{
    std::cout << "\n========================================\n";
    std::cout << "Example 3: Connection Status Handling\n";
    std::cout << "========================================\n\n";

    // Demonstrate all connection statuses
    std::cout << "[INFO] Available connection statuses:\n";
    std::cout << "  - " << connectionStatusToString(ConnectionStatus::Disconnected) 
              << ": Initial state, no connection\n";
    std::cout << "  - " << connectionStatusToString(ConnectionStatus::Connecting) 
              << ": Connection attempt in progress\n";
    std::cout << "  - " << connectionStatusToString(ConnectionStatus::Connected) 
              << ": Successfully connected\n";
    std::cout << "  - " << connectionStatusToString(ConnectionStatus::Error) 
              << ": Connection error occurred\n";
    std::cout << "  - " << connectionStatusToString(ConnectionStatus::Timeout) 
              << ": Operation timed out\n";
    std::cout << "  - " << connectionStatusToString(ConnectionStatus::Disconnecting) 
              << ": Graceful disconnect in progress\n";

    std::cout << "\n[TEST] Testing timeout behavior...\n";

    auto client = std::make_shared<NetworkClient>();
    
    int statusChanges = 0;
    client->setConnectionStatusCallback(
        [&statusChanges](ConnectionStatus status, const std::string& message) {
            statusChanges++;
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            
            std::cout << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") 
                     << "] Status #" << statusChanges << ": " 
                     << connectionStatusToString(status) 
                     << " - " << message << std::endl;
        });

    std::thread clientThread([client]() {
        client->run();
    });

    // Try to connect to non-existent server with short timeout
    std::cout << "\n[TEST] Attempting connection to non-existent server...\n";
    client->connectAsync("192.0.2.1", 9999, std::chrono::seconds(2));

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\n[TEST] Final status: " 
              << connectionStatusToString(client->getStatus()) << std::endl;
    std::cout << "[TEST] Total status changes: " << statusChanges << std::endl;

    client->stop();
    clientThread.join();

    std::cout << "\n[DONE] Example 3 completed\n";
}

// =============================================================================
// Main Function
// =============================================================================

int main(int argc, char* argv[])
{
    std::cout << "===========================================\n";
    std::cout << "SeaBattle Network Client Usage Examples\n";
    std::cout << "===========================================\n";
    std::cout << "\nNote: These examples expect a server at localhost:8080\n";
    std::cout << "Without a server, connection attempts will fail/timeout\n";

    try
    {
        // Run all examples
        example1_lowLevel();
        example2_highLevel();
        example3_statusDemo();

        std::cout << "\n===========================================\n";
        std::cout << "All examples completed successfully!\n";
        std::cout << "===========================================\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n[EXCEPTION] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
