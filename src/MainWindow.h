#pragma once

#include <QMainWindow>

class GameModelAdapter;
class WelcomeScreen;
class GameScreen;
class NetworkClient;
enum class ConnectionStatus;

namespace SeaBattle
{
    enum class CellState;
    enum class GameState;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    
    // Публичный метод для тестирования различных статусов подключения
    NetworkClient* getNetworkClient() const { return m_networkClient.get(); }

private slots:
    void showGameScreen();
    void showWelcomeScreen();
    void onCellClicked(int player, int row, int col);
    void onEnemyCellClicked(int row, int col);
    void onCellUpdated(int player, int row, int col, SeaBattle::CellState state);
    void onPlayerSwitched(int newPlayer);
    void onGameOver(int winner);
    void onGameStateChanged(SeaBattle::GameState state);
    void onExitGameRequested(); // Обработчик запроса на выход из игры
    void onConnectionStatusChanged(ConnectionStatus status); // Обработчик изменения статуса подключения

private:
    void initializeGameModel();
    void updateBattleFields();
    void showTurnMessage(int player);
    void refreshShipOverlaysForCurrentPlayer();

private:
    QStackedWidget* m_stackedWidget;
    WelcomeScreen* m_welcomeScreen;
    GameScreen* m_gameScreen;
    std::unique_ptr<GameModelAdapter> m_gameModel;
    std::unique_ptr<NetworkClient> m_networkClient;
};
