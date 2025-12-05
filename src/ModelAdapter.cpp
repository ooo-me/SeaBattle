#include "ModelAdapter.h"
#include "LocalGameModelSource.h"

GameModelAdapter::GameModelAdapter(std::unique_ptr<SeaBattle::IGameModelSource> source)
{
    if (source)
    {
        modelSource = std::move(source);
    }
    else
    {
        // По умолчанию используем локальный источник
        modelSource = std::make_unique<SeaBattle::LocalGameModelSource>();
    }

    // Устанавливаем обработчик событий
    modelSource->setEventCallback([this](const SeaBattle::GameEvent& event) {
        handleGameEvent(event);
    });
}

void GameModelAdapter::startGame()
{
    modelSource->startGame();
    if (gameStateCallback)
    {
        gameStateCallback(SeaBattle::GameState::Playing);
    }
}

bool GameModelAdapter::processShot(int row, int col)
{
    if (!modelSource->isValidShot(row, col))
    {
        return false;
    }

    return modelSource->processShot(row, col);
}

// Методы для получения состояния для GUI
SeaBattle::CellState GameModelAdapter::getPlayerCellState(int player, int row, int col) const
{
    return modelSource->getPlayerCellState(player, row, col);
}

SeaBattle::CellState GameModelAdapter::getEnemyCellState(int player, int row, int col) const
{
    return modelSource->getEnemyCellState(player, row, col);
}

const std::vector<SeaBattle::Ship>& GameModelAdapter::getPlayerShips(int player) const
{
    return modelSource->getPlayerShips(player);
}

void GameModelAdapter::setModelSource(std::unique_ptr<SeaBattle::IGameModelSource> source)
{
    modelSource = std::move(source);
    
    // Устанавливаем обработчик событий для нового источника
    modelSource->setEventCallback([this](const SeaBattle::GameEvent& event) {
        handleGameEvent(event);
    });
}

void GameModelAdapter::handleGameEvent(const SeaBattle::GameEvent& event)
{
    switch (event.type)
    {
    case SeaBattle::GameEventType::ShotResponse:
        // Уведомляем об изменении клетки
        // Получаем состояние через источник, который скрывает неподбитые корабли
        if (cellUpdateCallback)
        {
            SeaBattle::CellState newState = modelSource->getEnemyCellState(event.player, event.row, event.col);
            cellUpdateCallback(event.player, event.row, event.col, newState);
        }
        break;

    case SeaBattle::GameEventType::PlayerSwitch:
        // Уведомляем о смене игрока
        if (playerSwitchCallback)
        {
            playerSwitchCallback(event.player);
        }
        break;

    case SeaBattle::GameEventType::GameOver:
        // Уведомляем о завершении игры
        if (gameOverCallback)
        {
            gameOverCallback(event.winner);
        }
        break;

    case SeaBattle::GameEventType::Error:
        // Можно добавить логирование ошибок
        // В данной реализации просто игнорируем
        break;

    default:
        // Другие типы событий (ShipDestroyed, ShotRequest) 
        // можно обработать при необходимости
        break;
    }
}
