#pragma once

#include "IGameModelSource.h"
#include "Model.h"
#include <memory>

namespace SeaBattle
{
    // Локальная реализация источника игровой модели
    // Инкапсулирует прямую работу с GameModel для локальной игры
    class LocalGameModelSource : public IGameModelSource
    {
    public:
        LocalGameModelSource()
            : model(std::make_unique<GameModel>())
        {
        }

        void startGame() override
        {
            // Создаем новую модель с новой расстановкой кораблей
            model = std::make_unique<GameModel>();
            model->startGame();

            // Отправляем событие о смене игрока
            if (eventCallback)
            {
                eventCallback(GameEvent::PlayerSwitch(model->getCurrentPlayer()));
            }
        }

        bool processShot(int row, int col) override
        {
            if (!model->isValidShot(row, col))
            {
                if (eventCallback)
                {
                    eventCallback(GameEvent::Error("Invalid shot coordinates"));
                }
                return false;
            }

            int currentPlayer = model->getCurrentPlayer();
            bool hit = model->shoot(row, col);

            // Отправляем событие о результате выстрела
            if (eventCallback)
            {
                eventCallback(GameEvent::ShotResponse(currentPlayer, row, col, hit));
            }

            // Проверяем, уничтожен ли корабль
            if (hit)
            {
                CellState state = model->getEnemyViewCellState(currentPlayer, row, col);
                if (state == CellState::Destroyed)
                {
                    if (eventCallback)
                    {
                        eventCallback(GameEvent::ShipDestroyed(currentPlayer, row, col));
                    }
                }
            }

            // Проверяем состояние игры
            if (model->getGameState() == GameState::GameOver)
            {
                if (eventCallback)
                {
                    eventCallback(GameEvent::GameOver(model->getWinner()));
                }
            }
            else if (!hit)
            {
                // Если промах - смена игрока
                if (eventCallback)
                {
                    eventCallback(GameEvent::PlayerSwitch(model->getCurrentPlayer()));
                }
            }

            return hit;
        }

        CellState getPlayerCellState(int player, int row, int col) const override
        {
            return model->getPlayerCellState(player, row, col);
        }

        CellState getEnemyCellState(int player, int row, int col) const override
        {
            return model->getEnemyViewCellState(player, row, col);
        }

        const std::vector<Ship>& getPlayerShips(int player) const override
        {
            return model->getPlayerShips(player);
        }

        int getCurrentPlayer() const override
        {
            return model->getCurrentPlayer();
        }

        GameState getGameState() const override
        {
            return model->getGameState();
        }

        void setEventCallback(GameEventCallback callback) override
        {
            eventCallback = callback;
        }

        bool isValidShot(int row, int col) const override
        {
            return model->isValidShot(row, col);
        }

    private:
        std::unique_ptr<GameModel> model;
        GameEventCallback eventCallback;
    };

} // namespace SeaBattle
