#pragma once

#include <QWidget>

class BattleField;
namespace SeaBattle
{
    enum class CellState;
}

class GameScreen : public QWidget
{
    Q_OBJECT
public:
    explicit GameScreen(QWidget* parent = nullptr);

    BattleField* getPlayer1Field() const { return m_player1Field; }
    BattleField* getPlayer2Field() const { return m_player2Field; }

public slots:
    void onPlayerSwitched(int newPlayer);
    void onCellUpdated(int player, int row, int col, SeaBattle::CellState state);
    void onGameOver(int winner);

signals:
    void cellClicked(int player, int row, int col);
    void returnToMainMenu();

private slots:
    void onEnemyCellClicked(int row, int col);

private:
    void rebuildLayoutsForCurrentPlayer();

    BattleField* m_player1Field;
    BattleField* m_player2Field;
    int m_currentPlayer;

    QHBoxLayout* m_mainLayout;
    QVBoxLayout* m_leftLayout;
    QVBoxLayout* m_rightLayout;
    QLabel* m_player1Label;
    QLabel* m_player2Label;
};
