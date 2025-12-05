#pragma once

#include <QMainWindow>

class GameModelAdapter;
class WelcomeScreen;
class GameScreen;
class QProgressDialog;

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
    void onCreateNetworkGame(int port);
    void onJoinNetworkGame(const QString& host, int port);

private:
    void initializeGameModel();
    void updateBattleFields();
    void showTurnMessage(int player);
    void refreshShipOverlaysForCurrentPlayer();
    void showConnectionWaitingDialog(bool isHost, const QString& info);
    void hideConnectionWaitingDialog();

private:
    QStackedWidget* m_stackedWidget;
    WelcomeScreen* m_welcomeScreen;
    GameScreen* m_gameScreen;
    std::unique_ptr<GameModelAdapter> m_gameModel;
    QProgressDialog* m_connectionDialog;
    bool m_isNetworkGame;
};
