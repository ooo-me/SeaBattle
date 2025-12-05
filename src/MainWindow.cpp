#include "MainWindow.h"

#include "BattleField.h"
#include "GameScreen.h"
#include "ModelAdapter.h"
#include "WelcomeScreen.h"
#include "NetworkAdapter.h"
#include "ServerSettingsDialog.h"
#include "ClientSettingsDialog.h"
#include "Protocol.h"

#include <QStackedWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_isNetworkGame(false)
    , m_isServer(false)
    , m_serverDialog(nullptr)
    , m_clientDialog(nullptr)
{
    setWindowTitle("Морской Бой");
    setMinimumSize(1280, 720);

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Создаем экраны
    m_welcomeScreen = new WelcomeScreen();
    m_gameScreen = new GameScreen();

    m_stackedWidget->addWidget(m_welcomeScreen);
    m_stackedWidget->addWidget(m_gameScreen);

    initializeGameModel();

    // Подключаем сигналы
    connect(m_welcomeScreen, &WelcomeScreen::startGameRequested, this, &MainWindow::showGameScreen);
    connect(m_welcomeScreen, &WelcomeScreen::createServerRequested, this, &MainWindow::onCreateServerRequested);
    connect(m_welcomeScreen, &WelcomeScreen::joinGameRequested, this, &MainWindow::onJoinGameRequested);
    connect(m_gameScreen, &GameScreen::returnToMainMenu, this, &MainWindow::showWelcomeScreen);
    connect(m_gameScreen, &GameScreen::cellClicked, this, &MainWindow::onCellClicked);
    connect(m_gameScreen, &GameScreen::exitGameRequested, this, &MainWindow::onExitGameRequested);
}

MainWindow::~MainWindow() = default;

void MainWindow::showGameScreen()
{
    m_stackedWidget->setCurrentWidget(m_gameScreen);

    // Полный сброс визуального состояния перед новой игрой
    m_gameScreen->getPlayer1Field()->clearAll();
    m_gameScreen->getPlayer2Field()->clearAll();

    m_gameModel->startGame();
    updateBattleFields();
    showTurnMessage(m_gameModel->getCurrentPlayer());
    
    // Показываем кнопку выхода, так как игра началась (состояние Playing)
    m_gameScreen->setExitButtonVisible(true);
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
    int before = m_gameModel->getCurrentPlayer();
    if (player != before)
    {
        return;
    }

    // In network game, send shot to opponent
    if (m_isNetworkGame)
    {
        // Validate that it's our turn
        int ourPlayer = m_isServer ? 0 : 1;
        if (before != ourPlayer)
        {
            return;
        }
        
        // Check if shot is valid
        if (!m_gameModel->isValidShot(row, col))
        {
            return;
        }
        
        // Send shot to opponent
        m_networkAdapter->sendMessage(
            SeaBattle::Network::createShotMessage(row, col));
        
        // Disable cells while waiting for response
        m_gameScreen->getPlayer1Field()->disableAllCells();
        m_gameScreen->getPlayer2Field()->disableAllCells();
        
        return;
    }

    // Local game logic
    bool hit = m_gameModel->processShot(row, col);

    // Если попадание и игра не окончена — разрешаем ОСТАВШИЕСЯ незастреленные клетки на поле противника
    if (hit && m_gameModel->getGameState() == SeaBattle::GameState::Playing)
    {
        if (player == 0)
        {
            m_gameScreen->getPlayer2Field()->enableUnshotCells();
        }
        else
        {
            m_gameScreen->getPlayer1Field()->enableUnshotCells();
        }
        return;
    }

    // Если промах — игрок сменится через колбэк, и нужное поле активируется в GameScreen
    // Если выстрел недопустим (клик по уже обстрелянной клетке), модель вернет false и игрок НЕ сменится
    int after = m_gameModel->getCurrentPlayer();
    if (after == before && m_gameModel->getGameState() == SeaBattle::GameState::Playing)
    {
        // Недопустимый выстрел: возвращаем доступ к незастреленным клеткам поля противника
        if (before == 0)
        {
            m_gameScreen->getPlayer2Field()->enableUnshotCells();
        }
        else
        {
            m_gameScreen->getPlayer1Field()->enableUnshotCells();
        }
    }
}

void MainWindow::onEnemyCellClicked(int row, int col)
{
    if (m_gameModel->getGameState() != SeaBattle::GameState::Playing)
    {
        return;
    }

    // Этот слот не используется напрямую в текущей схеме, логика в onCellClicked
}

void MainWindow::onCellUpdated(int player, int row, int col, SeaBattle::CellState state)
{
    BattleField* targetField = nullptr;

    if (player == 0)
    {
        // Обновление на поле игрока 2 (противника для игрока 1)
        targetField = m_gameScreen->getPlayer2Field();
    }
    else
    {
        // Обновление на поле игрока 1 (противника для игрока 2)
        targetField = m_gameScreen->getPlayer1Field();
    }

    if (targetField)
    {
        switch (state)
        {
        case SeaBattle::CellState::Miss:
            targetField->markMiss(row, col);
            break;
        case SeaBattle::CellState::Hit:
        case SeaBattle::CellState::Destroyed:
            targetField->markHit(row, col);
            break;
        default:
            break;
        }
    }
}

void MainWindow::onPlayerSwitched(int newPlayer)
{
    showTurnMessage(newPlayer);

    refreshShipOverlaysForCurrentPlayer();
}

void MainWindow::onGameOver(int winner)
{
    m_gameScreen->getPlayer2Field()->disableAllCells();
    
    // Скрываем кнопку выхода, так как игра окончена
    m_gameScreen->setExitButtonVisible(false);

    QMessageBox msgBox;
    msgBox.setWindowTitle("Игра окончена");
    msgBox.setText(QString("Победил игрок %1!").arg(winner + 1));
    QPushButton* newGameButton = msgBox.addButton("Новая игра", QMessageBox::AcceptRole);
    QPushButton* exitButton = msgBox.addButton("Выход", QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == newGameButton)
    {
        // Перезапускаем игру
        m_gameModel = std::make_unique<GameModelAdapter>();
        initializeGameModel();
        // Очищаем поля сразу, чтобы на экране приветствия не оставались старые попадания
        m_gameScreen->getPlayer1Field()->clearAll();
        m_gameScreen->getPlayer2Field()->clearAll();
        showWelcomeScreen();
    }
    else if (msgBox.clickedButton() == exitButton)
    {
        qApp->quit();
    }
}

void MainWindow::onGameStateChanged(SeaBattle::GameState state)
{
}

void MainWindow::onExitGameRequested()
{
    // Завершаем текущую игру и возвращаемся на экран приветствия
    // Инициализируем новую модель для следующей игры
    initializeGameModel();
    
    // Очищаем игровые поля
    m_gameScreen->getPlayer1Field()->clearAll();
    m_gameScreen->getPlayer2Field()->clearAll();
    
    // Переходим на экран приветствия
    showWelcomeScreen();
}

void MainWindow::initializeGameModel()
{
    m_gameModel = std::make_unique<GameModelAdapter>();

    // Подключаем callback'и к GameScreen
    m_gameModel->setCellUpdateCallback([this](int player, int row, int col, SeaBattle::CellState state) {
        QMetaObject::invokeMethod(this, [this, player, row, col, state]() {
            m_gameScreen->onCellUpdated(player, row, col, state);
            });
        });

    m_gameModel->setPlayerSwitchCallback([this](int newPlayer) {
        QMetaObject::invokeMethod(this, [this, newPlayer]() {
            m_gameScreen->onPlayerSwitched(newPlayer);
            showTurnMessage(newPlayer);
            refreshShipOverlaysForCurrentPlayer();
            });
        });

    m_gameModel->setGameOverCallback([this](int winner) {
        QMetaObject::invokeMethod(this, [this, winner]() {
            m_gameScreen->onGameOver(winner);
            });
        });
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

    int current = m_gameModel->getCurrentPlayer();

    const auto& player1Ships = m_gameModel->getPlayerShips(0);
    const auto& player2Ships = m_gameModel->getPlayerShips(1);

    BattleField* ownField = nullptr;
    BattleField* enemyField = nullptr;
    const std::vector<SeaBattle::Ship>* ownShips = nullptr;
    const std::vector<SeaBattle::Ship>* enemyShips = nullptr;

    if (current == 0)
    {
        ownField = m_gameScreen->getPlayer1Field();
        enemyField = m_gameScreen->getPlayer2Field();
        ownShips = &player1Ships;
        enemyShips = &player2Ships;
    }
    else
    {
        ownField = m_gameScreen->getPlayer2Field();
        enemyField = m_gameScreen->getPlayer1Field();
        ownShips = &player2Ships;
        enemyShips = &player1Ships;
    }

    // Наносим зелёные метки для своих кораблей (ownField)
    for (const auto& ship : *ownShips)
    {
        for (const auto& pos : ship.positions)
        {
            ownField->markShip(pos.first, pos.second);
        }
    }

    // Подсветка кораблей противника (debug). Чтобы включить — поменять #if 0 на #if 1.
#if 0
    for (const auto& ship : *enemyShips)
    {
        for (const auto& pos : ship.positions)
        {
            enemyField->markDebug(pos.first, pos.second);
        }
    }
#endif
}

void MainWindow::showTurnMessage(int player)
{
    // Don't show turn message in network games
    if (m_isNetworkGame)
    {
        return;
    }
    
    QMessageBox msgBox;
    msgBox.setWindowTitle("Смена хода");
    msgBox.setText(QString("Ход игрока %1").arg(player + 1));
    msgBox.addButton("Продолжить", QMessageBox::AcceptRole);
    msgBox.exec();
}

void MainWindow::onCreateServerRequested()
{
    m_serverDialog = new ServerSettingsDialog(this);
    
    connect(m_serverDialog, &ServerSettingsDialog::startServerRequested, 
            this, [this](int port) {
        m_serverDialog->setWaitingForClient();
        
        m_networkAdapter = std::make_unique<SeaBattle::NetworkAdapter>();
        
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::connectionEstablished,
                this, &MainWindow::onNetworkConnectionEstablished);
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::messageReceived,
                this, &MainWindow::onNetworkMessageReceived);
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::connectionClosed,
                this, &MainWindow::onNetworkConnectionClosed);
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::errorOccurred,
                this, &MainWindow::onNetworkError);
        
        try {
            m_networkAdapter->startServer(port);
            m_isServer = true;
        } catch (const std::exception& e) {
            m_serverDialog->setError(QString::fromStdString(e.what()));
        }
    });
    
    m_serverDialog->exec();
}

void MainWindow::onJoinGameRequested()
{
    m_clientDialog = new ClientSettingsDialog(this);
    
    connect(m_clientDialog, &ClientSettingsDialog::connectRequested,
            this, [this](const QString& host, int port) {
        m_clientDialog->setConnecting();
        
        m_networkAdapter = std::make_unique<SeaBattle::NetworkAdapter>();
        
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::connectionEstablished,
                this, &MainWindow::onNetworkConnectionEstablished);
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::messageReceived,
                this, &MainWindow::onNetworkMessageReceived);
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::connectionClosed,
                this, &MainWindow::onNetworkConnectionClosed);
        connect(m_networkAdapter.get(), &SeaBattle::NetworkAdapter::errorOccurred,
                this, &MainWindow::onNetworkError);
        
        try {
            m_networkAdapter->connectToServer(host.toStdString(), port);
            m_isServer = false;
        } catch (const std::exception& e) {
            m_clientDialog->setError(QString::fromStdString(e.what()));
        }
    });
    
    m_clientDialog->exec();
}

void MainWindow::onNetworkConnectionEstablished()
{
    // Close dialogs
    if (m_serverDialog)
    {
        m_serverDialog->accept();
        m_serverDialog = nullptr;
    }
    if (m_clientDialog)
    {
        m_clientDialog->accept();
        m_clientDialog = nullptr;
    }
    
    // Start network game
    startNetworkGame(m_isServer);
}

void MainWindow::onNetworkMessageReceived(const SeaBattle::Network::Message& message)
{
    using namespace SeaBattle::Network;
    
    switch (message.type)
    {
        case MessageType::GameStart:
        {
            auto start = GameStartMessage::fromJson(message.payload);
            m_isServer = start.isServer;
            break;
        }
        
        case MessageType::Shot:
        {
            auto shot = ShotMessage::fromJson(message.payload);
            // Process opponent's shot
            bool hit = m_gameModel->processShot(shot.row, shot.col);
            
            // Send result back
            SeaBattle::CellState state = m_gameModel->getEnemyCellState(
                m_isServer ? 0 : 1, shot.row, shot.col);
            bool destroyed = (state == SeaBattle::CellState::Destroyed);
            
            m_networkAdapter->sendMessage(
                createShotResultMessage(shot.row, shot.col, hit, destroyed));
            break;
        }
        
        case MessageType::ShotResult:
        {
            auto result = ShotResultMessage::fromJson(message.payload);
            // Update UI with shot result
            SeaBattle::CellState state = result.destroyed ? 
                SeaBattle::CellState::Destroyed : 
                (result.hit ? SeaBattle::CellState::Hit : SeaBattle::CellState::Miss);
            
            int currentPlayer = m_isServer ? 0 : 1;
            onCellUpdated(currentPlayer, result.row, result.col, state);
            
            if (!result.hit)
            {
                // Switch turn
                onPlayerSwitched(m_isServer ? 1 : 0);
            }
            break;
        }
        
        case MessageType::GameOver:
        {
            auto gameOver = GameOverMessage::fromJson(message.payload);
            onGameOver(gameOver.winner);
            break;
        }
        
        case MessageType::Disconnect:
        {
            QMessageBox::information(this, "Соединение", 
                "Противник отключился от игры");
            showWelcomeScreen();
            cleanupNetwork();
            break;
        }
        
        case MessageType::Error:
        {
            auto error = ErrorMessage::fromJson(message.payload);
            QMessageBox::warning(this, "Ошибка", 
                QString::fromStdString(error.message));
            break;
        }
        
        default:
            break;
    }
}

void MainWindow::onNetworkConnectionClosed()
{
    if (m_isNetworkGame)
    {
        QMessageBox::information(this, "Соединение", 
            "Соединение с противником закрыто");
        showWelcomeScreen();
        cleanupNetwork();
    }
}

void MainWindow::onNetworkError(const QString& error)
{
    QMessageBox::critical(this, "Сетевая ошибка", error);
    
    if (m_serverDialog)
    {
        m_serverDialog->setError(error);
    }
    if (m_clientDialog)
    {
        m_clientDialog->setError(error);
    }
}

void MainWindow::startNetworkGame(bool isServer)
{
    m_isNetworkGame = true;
    m_isServer = isServer;
    
    // Send game start message
    m_networkAdapter->sendMessage(
        SeaBattle::Network::createGameStartMessage(isServer));
    
    // Start the game
    m_stackedWidget->setCurrentWidget(m_gameScreen);
    m_gameScreen->getPlayer1Field()->clearAll();
    m_gameScreen->getPlayer2Field()->clearAll();
    m_gameModel->startGame();
    updateBattleFields();
    
    m_gameScreen->setExitButtonVisible(true);
    
    // Server always starts first
    if (isServer)
    {
        m_gameScreen->getPlayer2Field()->enableUnshotCells();
    }
}

void MainWindow::cleanupNetwork()
{
    if (m_networkAdapter)
    {
        m_networkAdapter->stop();
        m_networkAdapter.reset();
    }
    m_isNetworkGame = false;
    m_isServer = false;
}
