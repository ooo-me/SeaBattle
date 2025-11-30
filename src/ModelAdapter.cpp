#include "ModelAdapter.h"


GameModelAdapter::GameModelAdapter()
    : model(std::make_unique<SeaBattle::GameModel>())
{
}

void GameModelAdapter::startGame()
{
    // Всегда начинаем с чистой модели и новой расстановкой кораблей
    model = std::make_unique<SeaBattle::GameModel>();
    model->startGame();
    if (gameStateCallback)
    {
        gameStateCallback(SeaBattle::GameState::Playing);
    }
    if (playerSwitchCallback)
    {
        playerSwitchCallback(model->getCurrentPlayer());
    }
}

bool GameModelAdapter::processShot(int row, int col)
{
    if (!model->isValidShot(row, col))
    {
        return false;
    }

    int currentPlayer = model->getCurrentPlayer();
    bool hit = model->shoot(row, col);

    // Уведомляем об изменении клетки
    if (cellUpdateCallback)
    {
        SeaBattle::CellState newState = model->getEnemyViewCellState(currentPlayer, row, col);
        cellUpdateCallback(currentPlayer, row, col, newState);
    }

    // Проверяем состояние игры
    if (model->getGameState() == SeaBattle::GameState::GameOver)
    {
        if (gameOverCallback)
        {
            gameOverCallback(model->getWinner());
        }
    }
    else if (!hit)
    {
        // Если промах - уведомляем о смене игрока
        if (playerSwitchCallback)
        {
            playerSwitchCallback(model->getCurrentPlayer());
        }
    }

    return hit;
}

// Методы для получения состояния для GUI
SeaBattle::CellState GameModelAdapter::getPlayerCellState(int player, int row, int col) const
{
    return model->getPlayerCellState(player, row, col);
}

SeaBattle::CellState GameModelAdapter::getEnemyCellState(int player, int row, int col) const
{
    return model->getEnemyViewCellState(player, row, col);
}

const std::vector<SeaBattle::Ship>& GameModelAdapter::getPlayerShips(int player) const
{
    return model->getPlayerShips(player);
}
