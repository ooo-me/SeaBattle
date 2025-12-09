#include "ModelAdapter.h"


GameModelAdapter::GameModelAdapter(SeaBattle::GameModel& sharedModel)
    : m_model(sharedModel)
{
}

void GameModelAdapter::StartGame()
{
    // Используем переданную модель
    m_model.StartGame();
    if (m_gameStateCallback)
    {
        m_gameStateCallback(SeaBattle::GameState::Playing);
    }
}

bool GameModelAdapter::ProcessShot(int row, int col)
{
    if (!m_model.isValidShot(row, col))
    {
        return false;
    }

    int currentPlayer = m_model.GetCurrentPlayer();
    bool hit = m_model.ProcessShot(row, col);

    // Уведомляем об изменении клетки
    if (m_cellUpdateCallback)
    {
        SeaBattle::CellState newState = m_model.getEnemyViewCellState(currentPlayer, row, col);
        m_cellUpdateCallback(currentPlayer, row, col, newState);
    }

    // Проверяем состояние игры
    if (m_model.GetGameState() == SeaBattle::GameState::GameOver)
    {
        if (m_gameOverCallback)
        {
            m_gameOverCallback(m_model.getWinner());
        }
    }
    else if (!hit)
    {
        // Если промах - уведомляем о смене игрока
        if (m_playerSwitchCallback)
        {
            m_playerSwitchCallback(m_model.GetCurrentPlayer());
        }
    }

    return hit;
}

// Методы для получения состояния для GUI
const std::vector<SeaBattle::Ship>& GameModelAdapter::GetPlayerShips(int player) const
{
    return m_model.GetPlayerShips(player);
}

SeaBattle::CellState GameModelAdapter::getPlayerCellState(int player, int row, int col) const
{
    return m_model.getPlayerCellState(player, row, col);
}

SeaBattle::CellState GameModelAdapter::getEnemyCellState(int player, int row, int col) const
{
    return m_model.getEnemyViewCellState(player, row, col);
}
