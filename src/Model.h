#pragma once

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

    class GameField
    {
    public:
        static const int SIZE = 10;

        GameField()
        {
            grid.fill(CellState::Empty);
            ships.reserve(10); // 1+2+3+4 = 10 кораблей
        }

        CellState getCellState(int row, int col) const
        {
            validateCoordinates(row, col);
            return grid[row * SIZE + col];
        }

        bool placeShip(const Ship& ship)
        {
            // Проверяем можно ли разместить корабль
            if (!canPlaceShip(ship))
            {
                return false;
            }

            // Размещаем корабль
            for (const auto& [row, col] : ship.positions)
            {
                grid[row * SIZE + col] = CellState::Ship;
            }

            ships.push_back(ship);
            return true;
        }

        bool canPlaceShip(const Ship& ship) const
        {
            for (const auto& [row, col] : ship.positions)
            {
                if (!isValidCoordinate(row, col))
                {
                    return false;
                }

                // Проверяем что клетка свободна и вокруг нет кораблей
                if (getCellState(row, col) != CellState::Empty)
                {
                    return false;
                }

                // Проверяем соседние клетки
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

        bool shoot(int row, int col)
        {
            validateCoordinates(row, col);

            CellState& cell = grid[row * SIZE + col];

            // Нельзя стрелять в уже обстрелянную клетку
            if (cell == CellState::Miss || cell == CellState::Hit || cell == CellState::Destroyed)
            {
                return false;
            }

            if (cell == CellState::Ship)
            {
                cell = CellState::Hit;

                // Находим корабль и уменьшаем его здоровье
                for (auto& ship : ships)
                {
                    auto it = std::find(ship.positions.begin(), ship.positions.end(),
                        std::make_pair(row, col));
                    if (it != ship.positions.end())
                    {
                        ship.health--;

                        // Если корабль уничтожен, помечаем все его клетки как Destroyed
                        if (ship.isDestroyed())
                        {
                            for (const auto& pos : ship.positions)
                            {
                                grid[pos.first * SIZE + pos.second] = CellState::Destroyed;
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

        bool allShipsDestroyed() const
        {
            return std::all_of(ships.begin(), ships.end(),
                [](const Ship& ship) { return ship.isDestroyed(); });
        }

        const std::vector<Ship>& getShips() const { return ships; }

    private:
        std::array<CellState, SIZE* SIZE> grid;
        std::vector<Ship> ships;

        bool isValidCoordinate(int row, int col) const
        {
            return row >= 0 && row < SIZE && col >= 0 && col < SIZE;
        }

        void validateCoordinates(int row, int col) const
        {
            if (!isValidCoordinate(row, col))
            {
                throw std::out_of_range("Invalid coordinates");
            }
        }
    };

    class ShipPlacer
    {
    public:
        static bool autoPlaceShips(GameField& field)
        {
            std::vector<ShipType> shipsToPlace = {
                ShipType::FourDeck,
                ShipType::TripleDeck, ShipType::TripleDeck,
                ShipType::DoubleDeck, ShipType::DoubleDeck, ShipType::DoubleDeck,
                ShipType::SingleDeck, ShipType::SingleDeck, ShipType::SingleDeck, ShipType::SingleDeck
            };

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

    private:
        static bool placeSingleShip(GameField& field, ShipType type, std::mt19937& gen)
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

                // Попробовать другую ориентацию
                vertical = !vertical;
            }

            return false;
        }
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
        virtual GameState GetGameState() const = 0;
    };

    class GameModel : public IModel
    {
    public:
        void StartGame() override
        {
            // Автоматически размещаем корабли для обоих игроков
            m_player1Field = GameField();
            m_player2Field = GameField();
            if (!ShipPlacer::autoPlaceShips(m_player1Field))
            {
                throw std::runtime_error("Failed to place ships for player 1");
            }
            if (!ShipPlacer::autoPlaceShips(m_player2Field))
            {
                throw std::runtime_error("Failed to place ships for player 2");
            }
            m_gameState = GameState::Playing;
            m_currentPlayer = 0; // Первый игрок начинает
            m_winner = 0;
        }

        bool ProcessShot(int row, int col) override
        {
            if (m_gameState != GameState::Playing)
            {
                return false;
            }

            GameField& enemyField = (m_currentPlayer == 0) ? m_player2Field : m_player1Field;
            bool hit = enemyField.shoot(row, col);

            if (hit)
            {
                // Проверяем уничтожен ли корабль
                checkShipDestruction(row, col, enemyField);

                // Проверяем конец игры
                if (enemyField.allShipsDestroyed())
                {
                    m_gameState = GameState::GameOver;
                    m_winner = m_currentPlayer;
                }
            }
            else
            {
                // Меняем игрока только при промахе
                switchPlayer();
            }

            return hit;
        }

        void switchPlayer()
        {
            m_currentPlayer = (m_currentPlayer + 1) % 2;
        }

        CellState getPlayerCellState(int player, int row, int col) const
        {
            const GameField& field = (player == 0) ? m_player1Field : m_player2Field;
            return field.getCellState(row, col);
        }

        CellState getEnemyCellState(int player, int row, int col) const
        {
            const GameField& field = (player == 0) ? m_player2Field : m_player1Field;
            return field.getCellState(row, col);
        }

        // Для отображения кораблей игрока
        const std::vector<Ship>& GetPlayerShips(int player) const override
        {
            const GameField& field = (player == 0) ? m_player1Field : m_player2Field;
            return field.getShips();
        }

        // Получить состояние конкретной клетки на поле противника (для GUI)
        CellState getEnemyViewCellState(int player, int row, int col) const
        {
            CellState state = getEnemyCellState(player, row, col);

            // Скрываем неподбитые корабли противника
            if (state == CellState::Ship)
            {
                return CellState::Empty;
            }

            return state;
        }

        int GetCurrentPlayer() const override { return m_currentPlayer; }
        GameState GetGameState() const override { return m_gameState; }
        int getWinner() const { return m_winner; }

        bool isValidShot(int row, int col) const
        {
            if (row < 0 || row >= GameField::SIZE || col < 0 || col >= GameField::SIZE)
            {
                return false;
            }

            const GameField& enemyField = (m_currentPlayer == 0) ? m_player2Field : m_player1Field;
            CellState state = enemyField.getCellState(row, col);

            // Можно стрелять только в Empty или Ship клетки
            return state == CellState::Empty || state == CellState::Ship;
        }

    private:
        GameField m_player1Field;
        GameField m_player2Field;
        int m_currentPlayer = 0;;
        GameState m_gameState = GameState::Welcome;
        int m_winner = 0;

        void checkShipDestruction(int row, int col, GameField& field)
        {
            // Логика проверки уничтожения корабля уже реализована в GameField::shoot
            // Эта функция оставлена для возможного расширения
        }
    };

} // namespace SeaBattle
