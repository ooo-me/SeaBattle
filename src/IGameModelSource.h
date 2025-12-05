#pragma once

#include "model.h"
#include <functional>
#include <memory>

namespace SeaBattle
{
    // Типы событий для сетевого взаимодействия
    enum class GameEventType
    {
        ShotRequest,      // Запрос выстрела
        ShotResponse,     // Ответ на выстрел (попадание/промах)
        ShipDestroyed,    // Корабль уничтожен
        GameOver,         // Игра завершена
        PlayerSwitch,     // Смена игрока
        Error             // Ошибка
    };

    // Структура события игры
    struct GameEvent
    {
        GameEventType type;
        int player;
        int row;
        int col;
        bool hit;
        int winner;
        std::string errorMessage;

        // Конструкторы для удобства
        static GameEvent ShotRequest(int player, int row, int col)
        {
            return { GameEventType::ShotRequest, player, row, col, false, -1, "" };
        }

        static GameEvent ShotResponse(int player, int row, int col, bool hit)
        {
            return { GameEventType::ShotResponse, player, row, col, hit, -1, "" };
        }

        static GameEvent ShipDestroyed(int player, int row, int col)
        {
            return { GameEventType::ShipDestroyed, player, row, col, false, -1, "" };
        }

        static GameEvent GameOver(int winner)
        {
            return { GameEventType::GameOver, -1, -1, -1, false, winner, "" };
        }

        static GameEvent PlayerSwitch(int newPlayer)
        {
            return { GameEventType::PlayerSwitch, newPlayer, -1, -1, false, -1, "" };
        }

        static GameEvent Error(const std::string& message)
        {
            return { GameEventType::Error, -1, -1, -1, false, -1, message };
        }
    };

    // Callback для обработки событий
    using GameEventCallback = std::function<void(const GameEvent& event)>;

    // Интерфейс источника игровой модели
    // Абстрагирует источник ходов (локальный или сетевой)
    class IGameModelSource
    {
    public:
        virtual ~IGameModelSource() = default;

        // Начать игру
        virtual void startGame() = 0;

        // Обработать выстрел
        virtual bool processShot(int row, int col) = 0;

        // Методы для получения состояния
        virtual CellState getPlayerCellState(int player, int row, int col) const = 0;
        virtual CellState getEnemyCellState(int player, int row, int col) const = 0;
        virtual const std::vector<Ship>& getPlayerShips(int player) const = 0;
        virtual int getCurrentPlayer() const = 0;
        virtual GameState getGameState() const = 0;

        // Установка callback для событий
        virtual void setEventCallback(GameEventCallback callback) = 0;

        // Проверка валидности хода
        virtual bool isValidShot(int row, int col) const = 0;
    };

} // namespace SeaBattle
