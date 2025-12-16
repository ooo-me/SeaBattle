#pragma once

#include <vector>

namespace SeaBattle
{
    enum class CellState
    {
        Empty,
        Ship,
        Miss,
        Hit,
        Destroyed
    };

    enum class ShipType
    {
        SingleDeck = 1,
        DoubleDeck = 2,
        TripleDeck = 3,
        FourDeck = 4
    };

    struct Ship
    {
        ShipType type;
        int health;
        std::vector<std::pair<int, int>> positions; // координаты корабля
        bool isVertical;

        Ship(ShipType t, int startRow, int startCol, bool vertical)
            : type(t)
            , health(static_cast<int>(t))
            , isVertical(vertical)
        {
            positions.reserve(health);
            for (int i = 0; i < health; ++i)
            {
                if (vertical)
                {
                    positions.emplace_back(startRow + i, startCol);
                }
                else
                {
                    positions.emplace_back(startRow, startCol + i);
                }
            }
        }

        bool isDestroyed() const { return health == 0; }
    };

    enum class GameState
    {
        Welcome,
        Playing,
        GameOver
    };

    struct IModel
    {
        virtual ~IModel() = default;

        virtual void StartGame() = 0;
        virtual bool ProcessShot(int row, int col) = 0;

        virtual const std::vector<SeaBattle::Ship>& GetPlayerShips(int player) const = 0;
        virtual int GetCurrentPlayer() const = 0;
        virtual int GetLocalPlayer() const = 0;
        virtual GameState GetGameState() const = 0;
    };
}
