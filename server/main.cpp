#include "GameModel.h"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <nlohmann/json.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <array>
#include <mutex>

namespace
{
    using WebSocketStream = boost::beast::websocket::stream<boost::beast::tcp_stream>;

    struct GameServerState
    {
        SeaBattle::GameModel model;
        int connectedPlayers = 0;
        bool gameStarted = false;
        
        // Хранение указателей на WebSocket-соединения игроков
        std::array<WebSocketStream*, 2> playerSockets = {nullptr, nullptr};
        std::mutex socketsMutex;

        // Player names
        std::array<std::string, 2> playerNames = {"Игрок 1", "Игрок 2"};
    };

    GameServerState g_state;

    nlohmann::json make_error(const std::string& msg)
    {
        std::cerr << "[server] error: " << msg << std::endl;
        return { {"type", "error"}, {"message", msg} };
    }

    nlohmann::json make_state(int playerIndex)
    {
        std::cout << "[server] make_state for player " << playerIndex << ": gameState="
                  << static_cast<int>(g_state.model.GetGameState())
                  << " currentPlayer=" << g_state.model.GetCurrentPlayer()
                  << " winner=" << g_state.model.GetWinner() << std::endl;

        nlohmann::json response = {
            {"type", "state"},
            {"gameState", static_cast<int>(g_state.model.GetGameState())},
            {"currentPlayer", g_state.model.GetCurrentPlayer()},
            {"winner", g_state.model.GetWinner()},
            {"playerNames", g_state.playerNames},
        };

        // Отправляем корабли игрока, если игра началась
        if (g_state.gameStarted)
        {
            const auto& playerField = g_state.model.GetPlayerField(playerIndex);
            nlohmann::json shipsJson = nlohmann::json::array();
            for (const auto& ship : playerField.getShips())
            {
                nlohmann::json shipJson;
                shipJson["type"] = static_cast<int>(ship.type);
                shipJson["health"] = ship.health;
                shipJson["isVertical"] = ship.isVertical;
                nlohmann::json positionsJson = nlohmann::json::array();
                for (const auto& pos : ship.positions)
                {
                    positionsJson.push_back({{"row", pos.first}, {"col", pos.second}});
                }
                shipJson["positions"] = positionsJson;
                shipsJson.push_back(shipJson);
            }
            response["ships"] = shipsJson;
        }

        return response;
    }

    // Отправка уведомления другому игроку о событии
    boost::asio::awaitable<void> notifyPlayer(int playerIndex, const nlohmann::json& message)
    {
        WebSocketStream* ws = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_state.socketsMutex);
            ws = g_state.playerSockets[playerIndex];
        }
        
        if (ws)
        {
            try
            {
                auto payload = message.dump();
                co_await ws->async_write(boost::asio::buffer(payload), boost::asio::use_awaitable);
                std::cout << "[server] notified player " << playerIndex << ": " << payload << std::endl;
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[server] failed to notify player " << playerIndex << ": " << ex.what() << std::endl;
            }
        }
    }

    boost::asio::awaitable<void> HandlePlayer(WebSocketStream ws, int playerIndex)
    {
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::server));
        co_await ws.async_accept();

        // Регистрируем соединение игрока
        {
            std::lock_guard<std::mutex> lock(g_state.socketsMutex);
            g_state.playerSockets[playerIndex] = &ws;
        }

        // Убираем регистрацию при выходе
        struct ScopeGuard {
            int idx;
            ~ScopeGuard() {
                std::lock_guard<std::mutex> lock(g_state.socketsMutex);
                g_state.playerSockets[idx] = nullptr;
            }
        } guard{playerIndex};

        {
            nlohmann::json hello{
                {"type", "hello"},
                {"player", playerIndex},
            };
            auto payload = hello.dump();
            co_await ws.async_write(boost::asio::buffer(payload), boost::asio::use_awaitable);
            std::cout << "[server] player " << playerIndex << " connected" << std::endl;
        }

        for (;;)
        {
            boost::beast::flat_buffer buffer;
            auto [ec, bytes] = co_await ws.async_read(
                buffer,
                boost::asio::as_tuple(boost::asio::use_awaitable));

            if (ec == boost::beast::websocket::error::closed)
            {
                std::cout << "[server] player " << playerIndex << " disconnected" << std::endl;
                co_return;
            }
            if (ec)
            {
                std::cerr << "[server] read error for player " << playerIndex
                          << ": " << ec.message() << std::endl;
                throw boost::system::system_error{ ec };
            }

            std::string msg{ boost::beast::buffers_to_string(buffer.data()) };
            std::cout << "[server] recv from player " << playerIndex
                      << " (" << bytes << " bytes): " << msg << std::endl;

            std::optional<nlohmann::json> o;
            try
            {
                o = nlohmann::json::parse(msg);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[server] json parse error: " << ex.what() << std::endl;
            }
            if (!o)
            {
                auto payload = make_error("invalid_json").dump();
                co_await ws.async_write(boost::asio::buffer(payload), boost::asio::use_awaitable);
                continue;
            }
            nlohmann::json request = *o;

            const std::string type = request.value("type", "");
            std::cout << "[server] request type='" << type << "' from player " << playerIndex << std::endl;

            if (type == "shot")
            {
                int row = request.value("row", -1);
                int col = request.value("col", -1);

                int previousPlayer = g_state.model.GetCurrentPlayer();
                bool hit = g_state.model.ProcessShot(playerIndex, row, col);
                int newPlayer = g_state.model.GetCurrentPlayer();

                std::cout << "[server] shot from player " << playerIndex
                          << " at (" << row << "," << col << ") hit=" << hit
                          << " gameState=" << static_cast<int>(g_state.model.GetGameState())
                          << " currentPlayer=" << g_state.model.GetCurrentPlayer()
                          << std::endl;

                nlohmann::json resp{
                    {"type", "shot_result"},
                    {"hit", hit},
                    {"row", row},
                    {"col", col},
                    {"currentPlayer", g_state.model.GetCurrentPlayer()},
                    {"gameState", static_cast<int>(g_state.model.GetGameState())},
                };

                if (g_state.model.GetGameState() == SeaBattle::GameState::GameOver)
                {
                    resp["winner"] = g_state.model.GetWinner();
                    std::cout << "[server] game over, winner=" << g_state.model.GetWinner() << std::endl;
                }

                auto payload = resp.dump();
                co_await ws.async_write(boost::asio::buffer(payload), boost::asio::use_awaitable);

                // Уведомляем другого игрока о выстреле
                int otherPlayer = 1 - playerIndex;
                nlohmann::json notification{
                    {"type", "opponent_shot"},
                    {"row", row},
                    {"col", col},
                    {"hit", hit},
                    {"currentPlayer", newPlayer},
                    {"gameState", static_cast<int>(g_state.model.GetGameState())},
                };
                if (g_state.model.GetGameState() == SeaBattle::GameState::GameOver)
                {
                    notification["winner"] = g_state.model.GetWinner();
                }
                co_await notifyPlayer(otherPlayer, notification);
            }
            else if (type == "state")
            {
                std::cout << "[server] state request from player " << playerIndex << std::endl;
                auto payload = make_state(playerIndex).dump();
                co_await ws.async_write(boost::asio::buffer(payload), boost::asio::use_awaitable);
            }
            else if (type == "set_name")
            {
                std::string name = request.value("name", "");
                if (!name.empty())
                {
                    g_state.playerNames[playerIndex] = name;
                    std::cout << "[server] player " << playerIndex << " set name to '" << name << "'" << std::endl;
                }
            }
            else
            {
                std::cerr << "[server] unknown request type='" << type << "' from player " << playerIndex << std::endl;
                auto payload = make_error("unknown_type").dump();
                co_await ws.async_write(boost::asio::buffer(payload), boost::asio::use_awaitable);
            }
        }
    }

    boost::asio::awaitable<void> DoSession(WebSocketStream stream)
    {
        if (g_state.connectedPlayers >= 2)
        {
            std::cout << "[server] reject connection: already 2 players" << std::endl;
            co_return; // only one game with two players
        }

        int assignedPlayer = g_state.connectedPlayers;
        ++g_state.connectedPlayers;
        std::cout << "[server] new session, assignedPlayer=" << assignedPlayer
                  << " totalPlayers=" << g_state.connectedPlayers << std::endl;

        if (!g_state.gameStarted && g_state.connectedPlayers == 2)
        {
            g_state.model.StartGame();
            g_state.gameStarted = true;
            std::cout << "[server] game started" << std::endl;
        }

        co_await HandlePlayer(std::move(stream), assignedPlayer);
    }

    boost::asio::awaitable<void> DoListen(boost::asio::ip::tcp::endpoint endpoint)
    {
        auto executor = co_await boost::asio::this_coro::executor;
        auto acceptor = boost::asio::ip::tcp::acceptor{ executor, endpoint };

        std::cout << "[server] listening on " << endpoint << std::endl;

        for (;;)
        {
            boost::asio::co_spawn(
                executor,
                DoSession(WebSocketStream{ co_await acceptor.async_accept(boost::asio::use_awaitable) }),
                [](std::exception_ptr e)
                {
                    if (e)
                    {
                        try
                        {
                            std::rethrow_exception(e);
                        }
                        catch (std::exception& ex)
                        {
                            std::cerr << "[server] Error in session: " << ex.what() << "\n";
                        }
                    }
                });
        }
    }
}

int main()
{
    auto const address = boost::asio::ip::make_address("127.0.0.7");
    auto const port = static_cast<unsigned short>(1365);

    boost::asio::thread_pool ioc(1);

    std::cout << "[server] starting, address=" << address.to_string()
              << " port=" << port << std::endl;

    boost::asio::co_spawn(
        ioc,
        DoListen(boost::asio::ip::tcp::endpoint{ address, port }),
        [](std::exception_ptr e)
        {
            if (e)
            {
                try
                {
                    std::rethrow_exception(e);
                }
                catch (std::exception const& ex)
                {
                    std::cerr << "[server] Error: " << ex.what() << std::endl;
                }
            }
        });

    ioc.wait();

    std::cout << "[server] stopped" << std::endl;
    return 0;
}
