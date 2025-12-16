#include "RemoteModel.h"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <nlohmann/json.hpp>

#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <boost/asio/thread_pool.hpp>

class Client
{
public:
    using PlayerSwitchCallback = std::function<void(int newPlayer)>;
    using CellUpdateCallback = std::function<void(int player, int row, int col, SeaBattle::CellState state)>;
    using GameOverCallback = std::function<void(bool)>;
    using StatusCallback = std::function<void(SeaBattle::ConnectionStatus status)>;
    using PlayerNamesCallback = std::function<void(const std::string& localName, const std::string& opponentName)>;

    Client()
        : m_ws(m_ioc)
    {
    }
    ~Client() = default;

    void setPlayerSwitchCallback(PlayerSwitchCallback callback) { m_playerSwitchCallback = callback; }
    void setCellUpdateCallback(CellUpdateCallback callback) { m_cellUpdateCallback = callback; }
    void setGameOverCallback(GameOverCallback callback) { m_gameOverCallback = callback; }
    void setStatusCallback(StatusCallback callback) { m_statusCallback = callback; }
    void setPlayerNamesCallback(PlayerNamesCallback callback) { m_playerNamesCallback = callback; }

    void setPlayerName(const std::string& name) { m_playerName = name; }

    bool connect()
    {
        return run_sync<bool>(
            [this]() -> boost::asio::awaitable<bool>
            {
                try
                {
                    auto executor = co_await boost::asio::this_coro::executor;
                    boost::asio::ip::tcp::resolver resolver(executor);
                    auto const results = co_await resolver.async_resolve("127.0.0.7", "1365", boost::asio::use_awaitable);

                    co_await boost::asio::async_connect(m_ws.next_layer(), results, boost::asio::use_awaitable);
                    co_await m_ws.async_handshake("127.0.0.7", "/", boost::asio::use_awaitable);

                    // Read hello message from server to get assigned player index
                    boost::beast::flat_buffer buffer;
                    auto [ec, bytes] = co_await m_ws.async_read(buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
                    if (!ec)
                    {
                        std::string msg{ boost::beast::buffers_to_string(buffer.data()) };
                        nlohmann::json hello = nlohmann::json::parse(msg, nullptr, false);
                        if (hello.is_object() && hello.value("type", "") == "hello")
                        {
                            std::lock_guard<std::mutex> lock(m_stateMutex);
                            m_localPlayer = hello.value("player", 0);
                        }
                    }

                    // Send player name to server
                    nlohmann::json setNameReq{
                        {"type", "set_name"},
                        {"name", m_playerName}
                    };
                    co_await m_ws.async_write(boost::asio::buffer(setNameReq.dump()), boost::asio::use_awaitable);

                    co_return true;
                }
                catch (...)
                {
                    co_return false;
                }
            });
    }

    bool request_state()
    {
        return run_sync<bool>(
            [this]() -> boost::asio::awaitable<bool>
            {
                try
                {
                    nlohmann::json req{ {"type", "state"} };
                    co_await m_ws.async_write(boost::asio::buffer(req.dump()), boost::asio::use_awaitable);

                    boost::beast::flat_buffer buffer;
                    auto [ec, bytes] = co_await m_ws.async_read(buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
                    if (ec)
                        co_return false;

                    std::string msg{ boost::beast::buffers_to_string(buffer.data()) };
                    nlohmann::json resp = nlohmann::json::parse(msg, nullptr, false);
                    if (!resp.is_object())
                        co_return false;

                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    m_currentPlayer = resp.value("currentPlayer", 0);
                    m_gameState = static_cast<SeaBattle::GameState>(resp.value("gameState", 0));
                    
                    if (resp.contains("ships") && resp["ships"].is_array())
                    {
                        m_ships.clear();
                        for (const auto& shipJson : resp["ships"])
                        {
                            SeaBattle::ShipType type = static_cast<SeaBattle::ShipType>(shipJson.value("type", 1));
                            bool isVertical = shipJson.value("isVertical", false);
                            if (shipJson.contains("positions") && shipJson["positions"].is_array() && !shipJson["positions"].empty())
                            {
                                int startRow = shipJson["positions"][0].value("row", 0);
                                int startCol = shipJson["positions"][0].value("col", 0);
                                m_ships.emplace_back(type, startRow, startCol, isVertical);
                            }
                        }
                    }

                    // Parse player names
                    if (resp.contains("playerNames") && resp["playerNames"].is_array())
                    {
                        auto& names = resp["playerNames"];
                        if (names.size() >= 2 && m_localPlayer >= 0 && m_localPlayer <= 1)
                        {
                            m_localPlayerName = names[m_localPlayer].get<std::string>();
                            m_opponentName = names[1 - m_localPlayer].get<std::string>();
                        }
                    }
                    
                    co_return true;
                }
                catch (...)
                {
                    co_return false;
                }
            });
    }

    bool wait_for_game_start()
    {
        // Report waiting status
        if (m_statusCallback)
        {
            m_statusCallback(SeaBattle::ConnectionStatus::WaitingForPlayers);
        }
            
        while (true)
        {
            if (!request_state())
            {
                return false;
            }
            
            if (game_state() == SeaBattle::GameState::Playing)
            {
                // Report loading status before game starts
                if (m_statusCallback)
                {
                    m_statusCallback(SeaBattle::ConnectionStatus::Loading);
                }

                // Notify about player names
                if (m_playerNamesCallback)
                {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    m_playerNamesCallback(m_localPlayerName, m_opponentName);
                }

                return true;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Отправка выстрела и ожидание результата через общий цикл чтения
    bool send_shot(int row, int col)
    {
        // Отправляем запрос
        auto sendResult = run_sync<bool>(
            [this, row, col]() -> boost::asio::awaitable<bool>
            {
                try
                {
                    nlohmann::json req{
                        {"type", "shot"},
                        {"row", row},
                        {"col", col},
                    };
                    co_await m_ws.async_write(boost::asio::buffer(req.dump()), boost::asio::use_awaitable);
                    co_return true;
                }
                catch (...)
                {
                    co_return false;
                }
            });

        if (!sendResult)
            return false;

        // Ждём ответа через условную переменную
        std::unique_lock<std::mutex> lock(m_shotMutex);
        m_shotResultReady = false;
        m_shotCV.wait(lock, [this]() { return m_shotResultReady; });
        return m_lastShotHit;
    }

    // Запуск цикла чтения входящих сообщений
    void startListening()
    {
        m_running.store(true);
        boost::asio::co_spawn(
            m_ioc,
            [this]() -> boost::asio::awaitable<void>
            {
                while (m_running.load())
                {
                    boost::beast::flat_buffer buffer;
                    auto [ec, bytes] = co_await m_ws.async_read(buffer, boost::asio::as_tuple(boost::asio::use_awaitable));
                    
                    if (ec)
                        break;

                    std::string msg{ boost::beast::buffers_to_string(buffer.data()) };
                    nlohmann::json resp = nlohmann::json::parse(msg, nullptr, false);
                    
                    if (!resp.is_object())
                        continue;

                    std::string type = resp.value("type", "");
                    
                    if (type == "shot_result")
                    {
                        // Ответ на наш выстрел
                        bool hit = resp.value("hit", false);
                        {
                            std::lock_guard<std::mutex> lock(m_stateMutex);
                            m_currentPlayer = resp.value("currentPlayer", 0);
                            m_gameState = static_cast<SeaBattle::GameState>(resp.value("gameState", 0));
                            if (resp.contains("winner"))
                            {
                                m_winner = resp.value("winner", -1);
                            }
                        }
                        
                        // Разблокируем send_shot
                        {
                            std::lock_guard<std::mutex> lock(m_shotMutex);
                            m_lastShotHit = hit;
                            m_shotResultReady = true;
                        }
                        m_shotCV.notify_one();
                    }
                    else if (type == "opponent_shot")
                    {
                        // Выстрел противника
                        int row = resp.value("row", -1);
                        int col = resp.value("col", -1);
                        bool hit = resp.value("hit", false);
                        int newPlayer = resp.value("currentPlayer", 0);
                        auto gameState = static_cast<SeaBattle::GameState>(resp.value("gameState", 0));

                        int previousPlayer;
                        {
                            std::lock_guard<std::mutex> lock(m_stateMutex);
                            previousPlayer = m_currentPlayer;
                            m_currentPlayer = newPlayer;
                            m_gameState = gameState;
                        }

                        if (m_cellUpdateCallback)
                        {
                            int opponent = 1 - local_player();
                            SeaBattle::CellState state = hit ? SeaBattle::CellState::Hit : SeaBattle::CellState::Miss;
                            m_cellUpdateCallback(opponent, row, col, state);
                        }

                        // Уведомляем о смене игрока только если ход действительно сменился
                        if (m_playerSwitchCallback && previousPlayer != newPlayer)
                        {
                            m_playerSwitchCallback(newPlayer);
                        }

                        if (gameState == SeaBattle::GameState::GameOver && m_gameOverCallback)
                        {
                            int winner = resp.value("winner", -1);
                            m_gameOverCallback(winner == m_localPlayer);
                        }
                    }
                }
            },
            [](std::exception_ptr) {});
    }

    int current_player() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_currentPlayer;
    }

    int local_player() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_localPlayer;
    }

    SeaBattle::GameState game_state() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_gameState;
    }

    int winner() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_winner;
    }

    const std::vector<SeaBattle::Ship>& ships() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_ships;
    }

    std::string local_player_name() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_localPlayerName;
    }

    std::string opponent_name() const
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_opponentName;
    }

private:
    template <typename T, typename Fn>
    T run_sync(Fn fn)
    {
        std::promise<T> p;
        auto fut = p.get_future();

        boost::asio::co_spawn(
            m_ioc,
            [fn = std::move(fn), &p]() -> boost::asio::awaitable<void>
            {
                try
                {
                    T val = co_await fn();
                    p.set_value(val);
                }
                catch (...)
                {
                    try { p.set_value(T{}); } catch (...) {}
                }
                co_return;
            },
            [](std::exception_ptr) {});

        return fut.get();
    }

    boost::asio::thread_pool m_ioc{ 1 };
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_ws;
    std::atomic<bool> m_running{false};

    mutable std::mutex m_stateMutex;
    int m_currentPlayer = 0;
    int m_localPlayer = 0;
    SeaBattle::GameState m_gameState = SeaBattle::GameState::Welcome;
    std::vector<SeaBattle::Ship> m_ships;
    int m_winner = -1;

    // Player names
    std::string m_playerName;
    std::string m_localPlayerName;
    std::string m_opponentName;

    // Для синхронизации результата выстрела
    std::mutex m_shotMutex;
    std::condition_variable m_shotCV;
    bool m_shotResultReady = false;
    bool m_lastShotHit = false;

    PlayerSwitchCallback m_playerSwitchCallback;
    CellUpdateCallback m_cellUpdateCallback;
    GameOverCallback m_gameOverCallback;
    StatusCallback m_statusCallback;
    PlayerNamesCallback m_playerNamesCallback;
};

RemoteModel::RemoteModel() = default;
RemoteModel::~RemoteModel() = default;

void RemoteModel::StartGame()
{
    m_client = std::make_unique<Client>();
    
    m_client->setPlayerSwitchCallback(m_playerSwitchCallback);
    m_client->setCellUpdateCallback(m_cellUpdateCallback);
    m_client->setGameOverCallback(m_gameOverCallback);
    m_client->setStatusCallback(m_statusCallback);
    m_client->setPlayerNamesCallback(m_playerNamesCallback);
    m_client->setPlayerName(m_playerName);
    
    m_client->connect();
    m_client->wait_for_game_start();
    m_client->startListening();
    
    // Notify that the game is ready
    if (m_gameReadyCallback)
    {
        m_gameReadyCallback();
    }
}

bool RemoteModel::ProcessShot(int row, int col)
{
    if (!m_client)
        return false;

    int localPlayer = m_client->local_player();
    int previousPlayer = m_client->current_player();
    const bool hit = m_client->send_shot(row, col);
    int newPlayer = m_client->current_player();

    if (m_cellUpdateCallback)
    {
        SeaBattle::CellState state = hit ? SeaBattle::CellState::Hit : SeaBattle::CellState::Miss;
        m_cellUpdateCallback(localPlayer, row, col, state);
    }

    if (m_playerSwitchCallback && previousPlayer != newPlayer)
    {
        m_playerSwitchCallback(newPlayer);
    }

    if (m_gameOverCallback && m_client->game_state() == SeaBattle::GameState::GameOver)
    {
        m_gameOverCallback(m_client->winner() == localPlayer);
    }

    return hit;
}

const std::vector<SeaBattle::Ship>& RemoteModel::GetPlayerShips(int player) const
{
    if (m_client && player == m_client->local_player())
    {
        return m_client->ships();
    }
    
    static std::vector<SeaBattle::Ship> empty;
    return empty;
}

int RemoteModel::GetCurrentPlayer() const
{
    if (!m_client)
        return 0;
    return m_client->current_player();
}

int RemoteModel::GetLocalPlayer() const
{
    if (!m_client)
        return 0;
    return m_client->local_player();
}

SeaBattle::GameState RemoteModel::GetGameState() const
{
    if (!m_client)
        return SeaBattle::GameState::Welcome;
    return m_client->game_state();
}

void RemoteModel::SetPlayerName(const std::string& name)
{
    m_playerName = name;
}

std::string RemoteModel::GetLocalPlayerName() const
{
    if (m_client)
    {
        std::string clientName = m_client->local_player_name();
        if (!clientName.empty())
            return clientName;
    }
    return m_playerName;
}

std::string RemoteModel::GetOpponentName() const
{
    if (!m_client)
        return "";
    return m_client->opponent_name();
}

