#include "GameProtocol.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>

using namespace SeaBattle::Protocol;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

int main()
{
    try
    {
        asio::io_context ioc;
        
        // Connect to server
        tcp::socket socket(ioc);
        tcp::resolver resolver(ioc);
        
        std::cout << "[Client] Connecting to server..." << std::endl;
        asio::connect(socket, resolver.resolve("localhost", "8080"));
        std::cout << "[Client] Connected!" << std::endl;
        
        // Send JOIN_GAME message
        JoinGameMessage join("TestPlayer");
        std::string msg = join.serialize();
        asio::write(socket, asio::buffer(msg));
        std::cout << "[Client] Sent: " << msg;
        
        // Read response
        asio::streambuf buffer;
        asio::read_until(socket, buffer, '\n');
        std::string response{
            asio::buffers_begin(buffer.data()),
            asio::buffers_begin(buffer.data()) + buffer.size()
        };
        std::cout << "[Client] Received: " << response;
        buffer.consume(buffer.size());
        
        // Send READY message
        ReadyMessage ready;
        msg = ready.serialize();
        asio::write(socket, asio::buffer(msg));
        std::cout << "[Client] Sent: " << msg;
        
        // Read response (YOUR_TURN)
        asio::read_until(socket, buffer, '\n');
        response = std::string{
            asio::buffers_begin(buffer.data()),
            asio::buffers_begin(buffer.data()) + buffer.size()
        };
        std::cout << "[Client] Received: " << response;
        buffer.consume(buffer.size());
        
        // Send a SHOOT message
        ShootMessage shoot(0, 0);
        msg = shoot.serialize();
        asio::write(socket, asio::buffer(msg));
        std::cout << "[Client] Sent: " << msg;
        
        // Read shoot result
        asio::read_until(socket, buffer, '\n');
        response = std::string{
            asio::buffers_begin(buffer.data()),
            asio::buffers_begin(buffer.data()) + buffer.size()
        };
        std::cout << "[Client] Received: " << response;
        buffer.consume(buffer.size());
        
        // Continue playing a few more shots
        for (int i = 1; i < 5; ++i)
        {
            // Read any pending messages (might be YOUR_TURN or OPPONENT_SHOT)
            while (buffer.size() > 0 || socket.available() > 0)
            {
                if (buffer.size() == 0)
                {
                    asio::read_until(socket, buffer, '\n');
                }
                
                response = std::string{
                    asio::buffers_begin(buffer.data()),
                    asio::buffers_begin(buffer.data()) + buffer.size()
                };
                std::cout << "[Client] Received: " << response;
                buffer.consume(buffer.size());
                
                if (response.find("YOUR_TURN") != std::string::npos)
                {
                    break;
                }
            }
            
            // Shoot
            ShootMessage nextShot(i, i);
            msg = nextShot.serialize();
            asio::write(socket, asio::buffer(msg));
            std::cout << "[Client] Sent: " << msg;
            
            // Read result
            asio::read_until(socket, buffer, '\n');
            response = std::string{
                asio::buffers_begin(buffer.data()),
                asio::buffers_begin(buffer.data()) + buffer.size()
            };
            std::cout << "[Client] Received: " << response;
            buffer.consume(buffer.size());
        }
        
        // Send QUIT
        QuitMessage quit;
        msg = quit.serialize();
        asio::write(socket, asio::buffer(msg));
        std::cout << "[Client] Sent: " << msg;
        
        std::cout << "[Client] Test completed successfully!" << std::endl;
        
        socket.close();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Client] Exception: " << e.what() << std::endl;
        return 1;
    }
}
