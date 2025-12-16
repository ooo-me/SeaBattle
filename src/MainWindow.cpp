#include "MainWindow.h"

#include "BattleField.h"
#include "GameScreen.h"
#include "WaitingScreen.h"
#include "WelcomeScreen.h"

#include <QStackedWidget>

MainWindow::MainWindow(SeaBattle::IModel& model, QWidget* parent)
    : QMainWindow(parent)
    , m_gameModel(model)
{
    setWindowTitle("Морской Бой");
    setMinimumSize(1280, 720);

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Создаем экраны
    m_welcomeScreen = new WelcomeScreen();
    m_waitingScreen = new WaitingScreen();
    m_gameScreen = new GameScreen();

    m_stackedWidget->addWidget(m_welcomeScreen);
    m_stackedWidget->addWidget(m_waitingScreen);
    m_stackedWidget->addWidget(m_gameScreen);

    // Подключаем сигналы
    connect(m_welcomeScreen, &WelcomeScreen::startGameRequested, this, &MainWindow::showWaitingScreen);
    connect(m_gameScreen, &GameScreen::returnToMainMenu, this, &MainWindow::showWelcomeScreen);
    connect(m_gameScreen, &GameScreen::cellClicked, this, &MainWindow::onCellClicked);
    connect(m_gameScreen, &GameScreen::exitGameRequested, this, &MainWindow::onExitGameRequested);
}

MainWindow::~MainWindow()
{
    if (m_connectionThread && m_connectionThread->joinable())
    {
        m_connectionThread->join();
    }
}

void MainWindow::showWaitingScreen(const QString& playerName)
{
    m_stackedWidget->setCurrentWidget(m_waitingScreen);
    m_waitingScreen->setStatusWaiting();
    
    // Set the player name in the model
    m_gameModel.SetPlayerName(playerName.toStdString());
    
    // Полный сброс визуального состояния перед новой игрой
    m_gameScreen->getPlayer1Field()->clearAll();
    m_gameScreen->getPlayer2Field()->clearAll();

    // Start the game connection in a background thread.
    // The status callback will update the waiting screen.
    // The game ready callback will transition to the game screen.
    // Ensure any previous connection thread is finished before starting a new one.
    if (m_connectionThread && m_connectionThread->joinable())
    {
        m_connectionThread->join();
    }
    m_connectionThread = std::make_unique<std::thread>([this]() {
        m_gameModel.StartGame();
    });
}

void MainWindow::showGameScreen()
{
    m_stackedWidget->setCurrentWidget(m_gameScreen);
    
    // Устанавливаем локального игрока и текущего игрока в GameScreen
    m_gameScreen->setLocalPlayer(m_gameModel.GetLocalPlayer());
    m_gameScreen->onPlayerSwitched(m_gameModel.GetCurrentPlayer());
    
    updateBattleFields();
    showTurnMessage(m_gameModel.GetCurrentPlayer());

    // Показываем кнопку выхода, так как игра началась (состояние Playing)
    m_gameScreen->setExitButtonVisible(true);
}

void MainWindow::onStatusUpdate(SeaBattle::ConnectionStatus status)
{
    if (status == SeaBattle::ConnectionStatus::WaitingForPlayers)
    {
        m_waitingScreen->setStatusWaiting();
    }
    else if (status == SeaBattle::ConnectionStatus::Loading)
    {
        m_waitingScreen->setStatusLoading();
    }
}

void MainWindow::onGameReady()
{
    showGameScreen();
}

void MainWindow::onPlayerNamesReceived(const std::string& localName, const std::string& opponentName)
{
    m_gameScreen->setPlayerNames(QString::fromStdString(localName), QString::fromStdString(opponentName));
}

void MainWindow::showWelcomeScreen()
{
    m_stackedWidget->setCurrentWidget(m_welcomeScreen);

    // Скрываем кнопку выхода на экране приветствия
    m_gameScreen->setExitButtonVisible(false);
}

void MainWindow::onCellClicked(int player, int row, int col)
{
    // Текущий игрок до выстрела
    int before = m_gameModel.GetCurrentPlayer();
    if (player != before)
    {
        return;
    }

    bool hit = m_gameModel.ProcessShot(row, col);

    // Если попадание и игра не окончена — разрешаем ОСТАВШИЕСЯ незастреленные клетки на поле противника
    if (hit && m_gameModel.GetGameState() == SeaBattle::GameState::Playing)
    {
        // m_player2Field - всегда поле противника
        m_gameScreen->getPlayer2Field()->enableUnshotCells();
        return;
    }

    // Если промах — игрок сменится через колбэк, и нужное поле активируется в GameScreen
    // Если выстрел недопустим (клик по уже обстрелянной клетке), модель вернет false и игрок НЕ сменится
    int after = m_gameModel.GetCurrentPlayer();
    if (after == before && m_gameModel.GetGameState() == SeaBattle::GameState::Playing)
    {
        // Недопустимый выстрел: возвращаем доступ к незастреленным клеткам поля противника
        m_gameScreen->getPlayer2Field()->enableUnshotCells();
    }
}

void MainWindow::onCellUpdated(int player, int row, int col, SeaBattle::CellState state)
{
    m_gameScreen->onCellUpdated(player, row, col, state);
}

void MainWindow::onPlayerSwitched(int newPlayer)
{
    m_gameScreen->onPlayerSwitched(newPlayer);
    showTurnMessage(newPlayer);
    refreshShipOverlaysForCurrentPlayer();
}

void MainWindow::onGameOver(bool win)
{
    m_gameScreen->onGameOver(win);
}

void MainWindow::onExitGameRequested()
{
    // Очищаем игровые поля
    m_gameScreen->getPlayer1Field()->clearAll();
    m_gameScreen->getPlayer2Field()->clearAll();

    // Переходим на экран приветствия
    showWelcomeScreen();
}

void MainWindow::updateBattleFields()
{
    refreshShipOverlaysForCurrentPlayer();
}

void MainWindow::refreshShipOverlaysForCurrentPlayer()
{
    // Сбрасываем стили незастреленных клеток на обоих полях
    m_gameScreen->getPlayer1Field()->resetUnfiredCellsStyle();
    m_gameScreen->getPlayer2Field()->resetUnfiredCellsStyle();

    int localPlayer = m_gameModel.GetLocalPlayer();

    // Получаем корабли локального игрока
    const auto& localShips = m_gameModel.GetPlayerShips(localPlayer);

    // m_player1Field - всегда "Ваше поле", поэтому корабли рисуем там
    BattleField* ownField = m_gameScreen->getPlayer1Field();

    // Наносим зелёные метки для своих кораблей
    for (const auto& ship : localShips)
    {
        for (const auto& pos : ship.positions)
        {
            ownField->markShip(pos.first, pos.second);
        }
    }
}

void MainWindow::showTurnMessage(int currentPlayer)
{
    int localPlayer = m_gameModel.GetLocalPlayer();
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Смена хода");
    
    if (currentPlayer == localPlayer)
    {
        msgBox.setText("Ваш ход");
    }
    else
    {
        msgBox.setText("Ход противника");
    }
    
    msgBox.addButton("Продолжить", QMessageBox::AcceptRole);
    
    // Центрируем окно сообщения относительно родительского окна
    msgBox.move(this->geometry().center() - msgBox.rect().center());
    
    msgBox.exec();
}
