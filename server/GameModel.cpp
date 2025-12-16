#include "GameModel.h"

namespace SeaBattle
{
    GameField::GameField()
    {
        m_grid.fill(CellState::Empty);
        m_ships.reserve(10);
    }

    CellState GameField::getCellState(int row, int col) const
    {
        validateCoordinates(row, col);
        return m_grid[row * SIZE + col];
    }

    bool GameField::placeShip(const Ship& ship)
    {
        if (!canPlaceShip(ship))
        {
            return false;
        }

        for (const auto& [row, col] : ship.positions)
        {
            m_grid[row * SIZE + col] = CellState::Ship;
        }

        m_ships.push_back(ship);
        return true;
    }

    bool GameField::canPlaceShip(const Ship& ship) const
    {
        for (const auto& [row, col] : ship.positions)
        {
            if (!isValidCoordinate(row, col))
            {
                return false;
            }

            if (getCellState(row, col) != CellState::Empty)
            {
                return false;
            }

            for (int dr = -1; dr <= 1; ++dr)
            {
                for (int dc = -1; dc <= 1; ++dc)
                {
                    int nr = row + dr;
                    int nc = col + dc;
                    if (isValidCoordinate(nr, nc) && getCellState(nr, nc) == CellState::Ship)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool GameField::shoot(int row, int col)
    {
        validateCoordinates(row, col);

        CellState& cell = m_grid[row * SIZE + col];

        if (cell == CellState::Miss || cell == CellState::Hit || cell == CellState::Destroyed)
        {
            return false;
        }

        if (cell == CellState::Ship)
        {
            cell = CellState::Hit;

            for (auto& ship : m_ships)
            {
                auto it = std::find(ship.positions.begin(), ship.positions.end(), std::make_pair(row, col));
                if (it != ship.positions.end())
                {
                    ship.health--;

                    if (ship.isDestroyed())
                    {
                        for (const auto& pos : ship.positions)
                        {
                            m_grid[pos.first * SIZE + pos.second] = CellState::Destroyed;
                        }
                    }

                    return true;
                }
            }
        }
        else
        {
            cell = CellState::Miss;
        }

        return false;
    }

    bool GameField::allShipsDestroyed() const
    {
        return std::all_of(m_ships.begin(), m_ships.end(), [](const Ship& ship) { return ship.isDestroyed(); });
    }

    bool GameField::isValidCoordinate(int row, int col) const
    {
        return row >= 0 && row < SIZE && col >= 0 && col < SIZE;
    }

    void GameField::validateCoordinates(int row, int col) const
    {
        if (!isValidCoordinate(row, col))
        {
            throw std::out_of_range("Invalid coordinates");
        }
    }

    bool ShipPlacer::autoPlaceShips(GameField& field)
    {
        std::vector<ShipType> shipsToPlace = {
            ShipType::FourDeck,
            ShipType::TripleDeck, ShipType::TripleDeck,
            ShipType::DoubleDeck, ShipType::DoubleDeck, ShipType::DoubleDeck,
            ShipType::SingleDeck, ShipType::SingleDeck, ShipType::SingleDeck, ShipType::SingleDeck};

        std::random_device rd;
        std::mt19937 gen(rd());

        for (ShipType type : shipsToPlace)
        {
            if (!placeSingleShip(field, type, gen))
            {
                return false;
            }
        }

        return true;
    }

    bool ShipPlacer::placeSingleShip(GameField& field, ShipType type, std::mt19937& gen)
    {
        std::uniform_int_distribution<> dist(0, 1);
        bool vertical = dist(gen) == 0;

        int maxAttempts = 100;

        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            int maxRow = GameField::SIZE - (vertical ? static_cast<int>(type) : 1);
            int maxCol = GameField::SIZE - (vertical ? 1 : static_cast<int>(type));

            if (maxRow < 0 || maxCol < 0)
                continue;

            std::uniform_int_distribution<> rowDist(0, maxRow);
            std::uniform_int_distribution<> colDist(0, maxCol);

            int startRow = rowDist(gen);
            int startCol = colDist(gen);

            Ship ship(type, startRow, startCol, vertical);

            if (field.canPlaceShip(ship))
            {
                if (field.placeShip(ship))
                {
                    return true;
                }
            }

            vertical = !vertical;
        }

        return false;
    }

    GameModel::GameModel()
        : m_currentPlayer(0)
        , m_gameState(GameState::WaitingForPlayers)
        , m_winner(-1)
    {
    }

    void GameModel::StartGame()
    {
        m_playerFields[0] = GameField();
        m_playerFields[1] = GameField();

        if (!ShipPlacer::autoPlaceShips(m_playerFields[0]))
        {
            throw std::runtime_error("Failed to place ships for player 1");
        }

        if (!ShipPlacer::autoPlaceShips(m_playerFields[1]))
        {
            throw std::runtime_error("Failed to place ships for player 2");
        }

        m_gameState = GameState::Playing;
        m_currentPlayer = 0;
        m_winner = -1;
    }

    bool GameModel::ProcessShot(int playerIndex, int row, int col)
    {
        if (m_gameState != GameState::Playing)
        {
            return false;
        }

        if (playerIndex != m_currentPlayer)
        {
            return false;
        }

        GameField& enemyField = m_playerFields[(playerIndex + 1) % 2];
        bool hit = enemyField.shoot(row, col);

        if (hit)
        {
            if (enemyField.allShipsDestroyed())
            {
                m_gameState = GameState::GameOver;
                m_winner = playerIndex;
            }
        }
        else
        {
            switchPlayer();
        }

        return hit;
    }

    const GameField& GameModel::GetPlayerField(int playerIndex) const
    {
        return m_playerFields[playerIndex];
    }

    const GameField& GameModel::GetEnemyField(int playerIndex) const
    {
        return m_playerFields[(playerIndex + 1) % 2];
    }

    void GameModel::switchPlayer()
    {
        m_currentPlayer = (m_currentPlayer + 1) % 2;
    }
}
