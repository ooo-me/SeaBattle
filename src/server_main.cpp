#include "GameServer.h"
#include <iostream>
#include <csignal>
#include <memory>

namespace
{
    std::unique_ptr<SeaBattle::GameServer> g_server;
    boost::asio::io_context* g_ioc = nullptr;
}

void signalHandler(int signal)
{
    std::cout << "\n[Server] Shutting down gracefully..." << std::endl;
    
    if (g_server)
    {
        g_server->stop();
    }
    
    if (g_ioc)
    {
        g_ioc->stop();
    }
}

int main(int argc, char* argv[])
{
    try
    {
        unsigned short port = 8080;
        
        if (argc > 1)
        {
            port = static_cast<unsigned short>(std::atoi(argv[1]));
        }
        
        std::cout << "[Server] SeaBattle Server starting on port " << port << std::endl;
        
        // Set up signal handling
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        boost::asio::io_context ioc;
        g_ioc = &ioc;
        
        g_server = std::make_unique<SeaBattle::GameServer>(ioc, port);
        g_server->start();
        
        // Run the io_context
        ioc.run();
        
        std::cout << "[Server] Server stopped" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Server] Exception: " << e.what() << std::endl;
        return 1;
    }
}
