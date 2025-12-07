#include "GameServer.h"
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    try
    {
        unsigned short port = 8080;
        
        if (argc > 1)
        {
            try
            {
                int portValue = std::stoi(argv[1]);
                if (portValue > 0 && portValue <= 65535)
                {
                    port = static_cast<unsigned short>(portValue);
                }
                else
                {
                    std::cerr << "[Server] Invalid port number. Using default port 8080." << std::endl;
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[Server] Invalid port argument. Using default port 8080." << std::endl;
            }
        }
        
        std::cout << "[Server] SeaBattle Server starting on port " << port << std::endl;
        
        boost::asio::io_context ioc;
        
        // Set up signal handling using boost::asio::signal_set (signal-safe)
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        
        auto server = std::make_shared<SeaBattle::GameServer>(ioc, port);
        server->start();
        
        // Handle signals asynchronously
        signals.async_wait([&server, &ioc](boost::system::error_code /*ec*/, int /*signo*/) {
            std::cout << "\n[Server] Shutting down gracefully..." << std::endl;
            server->stop();
            ioc.stop();
        });
        
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
