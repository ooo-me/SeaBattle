#pragma once

#include <QWidget>

class BattleField;
namespace SeaBattle
{
    enum class CellState;
}

enum class ConnectionStatus
{
    Waiting,      // Ожидание подключения
    Connected,    // Успешное подключение
    Error,        // Ошибка подключения
    Timeout,      // Таймаут подключения
    Disconnected  // Разрыв соединения
};

class GameScreen : public QWidget
{
    Q_OBJECT
public:
    explicit GameScreen(QWidget* parent = nullptr);

    BattleField* getPlayer1Field() const { return m_player1Field; }
    BattleField* getPlayer2Field() const { return m_player2Field; }
    
    // Показать/скрыть кнопку выхода в зависимости от состояния игры
    void setExitButtonVisible(bool visible);
    
    // Установить статус подключения
    void setConnectionStatus(ConnectionStatus status);

public slots:
    void onPlayerSwitched(int newPlayer);
    void onCellUpdated(int player, int row, int col, SeaBattle::CellState state);
    void onGameOver(int winner);

signals:
    void cellClicked(int player, int row, int col);
    void returnToMainMenu();
    void exitGameRequested(); // Сигнал для выхода из игры

private slots:
    void onEnemyCellClicked(int row, int col);
    void onExitButtonClicked(); // Обработчик нажатия кнопки выхода

private:
    void rebuildLayoutsForCurrentPlayer();

    BattleField* m_player1Field;
    BattleField* m_player2Field;
    int m_currentPlayer;

    QVBoxLayout* m_mainLayout; // Главный вертикальный layout
    QHBoxLayout* m_fieldsLayout; // Горизонтальный layout для полей
    QVBoxLayout* m_leftLayout;
    QVBoxLayout* m_rightLayout;
    QHBoxLayout* m_buttonsLayout; // Горизонтальный layout для кнопок внизу
    QWidget* m_buttonsWidget; // Контейнер для layout кнопок
    QLabel* m_player1Label;
    QLabel* m_player2Label;
    QPushButton* m_exitButton; // Кнопка выхода из игры
    QLabel* m_statusBar; // Статус бар для отображения состояния подключения
};
