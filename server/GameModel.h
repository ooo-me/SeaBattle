#pragma once

#include <array>
#include <random>
#include <stdexcept>
#include <utility>
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
        std::vector<std::pair<int, int>> positions;
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

    class GameField
    {
    public:
        static const int SIZE = 10;

        GameField();

        CellState getCellState(int row, int col) const;
        bool placeShip(const Ship& ship);
        bool canPlaceShip(const Ship& ship) const;
        bool shoot(int row, int col);
        bool allShipsDestroyed() const;
        const std::vector<Ship>& getShips() const { return m_ships; }

    private:
        std::array<CellState, SIZE * SIZE> m_grid{};
        std::vector<Ship> m_ships;

        bool isValidCoordinate(int row, int col) const;
        void validateCoordinates(int row, int col) const;
    };

    class ShipPlacer
    {
    public:
        static bool autoPlaceShips(GameField& field);

    private:
        static bool placeSingleShip(GameField& field, ShipType type, std::mt19937& gen);
    };

    enum class GameState
    {
        WaitingForPlayers,
        Playing,
        GameOver
    };

    class GameModel
    {
    public:
        GameModel();

        void StartGame();
        bool ProcessShot(int playerIndex, int row, int col);

        int GetCurrentPlayer() const { return m_currentPlayer; }
        GameState GetGameState() const { return m_gameState; }
        int GetWinner() const { return m_winner; }

        const GameField& GetPlayerField(int playerIndex) const;
        const GameField& GetEnemyField(int playerIndex) const;

    private:
        GameField m_playerFields[2];
        int m_currentPlayer;
        GameState m_gameState;
        int m_winner;

        void switchPlayer();
    };
}
