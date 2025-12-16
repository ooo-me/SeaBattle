#pragma once

#include "IModel.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QString>
#include <memory>
#include <thread>

class WelcomeScreen;
class WaitingScreen;
class GameScreen;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(SeaBattle::IModel& model, QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    void onCellUpdated(int player, int row, int col, SeaBattle::CellState state);
    void onPlayerSwitched(int newPlayer);
    void onGameOver(bool win);
    void onStatusUpdate(SeaBattle::ConnectionStatus status);
    void onGameReady();
    void onPlayerNamesReceived(const QString& localName, const QString& opponentName);

private slots:
    void showWaitingScreen(const QString& playerName);
    void showGameScreen();
    void showWelcomeScreen();
    void onCellClicked(int player, int row, int col);
    void onExitGameRequested(); // Обработчик запроса на выход из игры

private:
    void updateBattleFields();
    void showTurnMessage(int player);
    void refreshShipOverlaysForCurrentPlayer();

private:
    QStackedWidget* m_stackedWidget;
    WelcomeScreen* m_welcomeScreen;
    WaitingScreen* m_waitingScreen;
    GameScreen* m_gameScreen;
    SeaBattle::IModel& m_gameModel;
    std::unique_ptr<std::thread> m_connectionThread;
};
