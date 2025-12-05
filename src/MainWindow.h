#pragma once

#include <QMainWindow>
#include <memory>

class GameModelAdapter;
class WelcomeScreen;
class GameScreen;
class ServerSettingsDialog;
class ClientSettingsDialog;
namespace SeaBattle
{
    enum class CellState;
    enum class GameState;
    class NetworkAdapter;
    namespace Network
    {
        struct Message;
    }
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

    // Network game slots
    void onCreateServerRequested();
    void onJoinGameRequested();
    void onNetworkMessageReceived(const SeaBattle::Network::Message& message);
    void onNetworkConnectionEstablished();
    void onNetworkConnectionClosed();
    void onNetworkError(const QString& error);

private:
    void initializeGameModel();
    void updateBattleFields();
    void showTurnMessage(int player);
    void refreshShipOverlaysForCurrentPlayer();
    void startNetworkGame(bool isServer);
    void cleanupNetwork();

private:
    QStackedWidget* m_stackedWidget;
    WelcomeScreen* m_welcomeScreen;
    GameScreen* m_gameScreen;
    std::unique_ptr<GameModelAdapter> m_gameModel;
    std::unique_ptr<SeaBattle::NetworkAdapter> m_networkAdapter;
    
    bool m_isNetworkGame;
    bool m_isServer;
    ServerSettingsDialog* m_serverDialog;
    ClientSettingsDialog* m_clientDialog;
};
