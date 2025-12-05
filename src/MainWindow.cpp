#include "MainWindow.h"

#include "BattleField.h"
#include "GameScreen.h"
#include "ModelAdapter.h"
#include "WelcomeScreen.h"
#include "NetworkAdapter.h"
#include "NetworkGameModel.h"
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
    // Initialize enemy field state
    m_enemyFieldState.fill(SeaBattle::CellState::Empty);
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
    // This is for local game mode
    m_isNetworkGame = false;
    
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
    // In network game, send shot to opponent
    if (m_isNetworkGame)
    {
        // In network mode, player field mapping is different
        // We need to determine which field was clicked based on the field itself
        // Player 2 field is always the enemy field (on the right)
        // The 'player' parameter from GameScreen may not be reliable in network mode
        // So we check if the cell was already shot in our enemy field state
        
        int idx = row * 10 + col;
        SeaBattle::CellState state = m_enemyFieldState[idx];
        if (state != SeaBattle::CellState::Empty)
        {
            // Already shot this cell
            return;
        }
        
        // Send shot to opponent
        m_networkAdapter->sendMessage(
            SeaBattle::Network::createShotMessage(row, col));
        
        // Disable cells while waiting for response
        m_gameScreen->getPlayer2Field()->disableAllCells();
        
        return;
    }

    // Local game logic
    int before = m_gameModel->getCurrentPlayer();
    if (player != before)
    {
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
    
    // If it's a network game, send disconnect message and cleanup
    if (m_isNetworkGame && m_networkAdapter)
    {
        m_networkAdapter->sendMessage(SeaBattle::Network::createDisconnectMessage());
        cleanupNetwork();
    }
    
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
            // This is just confirmation, we already know our role
            break;
        }
        
        case MessageType::Shot:
        {
            auto shot = ShotMessage::fromJson(message.payload);
            // Opponent is shooting at our field
            
            bool destroyed = false;
            bool hit = m_networkGameModel->processOpponentShot(shot.row, shot.col, destroyed);
            
            // Update our field display
            BattleField* myField = m_gameScreen->getPlayer1Field(); // We always see ourselves on left
            if (hit)
            {
                myField->markHit(shot.row, shot.col);
            }
            else
            {
                myField->markMiss(shot.row, shot.col);
            }
            
            // Send result back
            m_networkAdapter->sendMessage(
                createShotResultMessage(shot.row, shot.col, hit, destroyed));
            
            // Check if we lost
            if (m_networkGameModel->allShipsDestroyed())
            {
                int winner = m_isServer ? 1 : 0; // Opponent wins
                m_networkAdapter->sendMessage(createGameOverMessage(winner));
                onGameOver(winner);
            }
            
            // If opponent hit, they continue, otherwise it's our turn
            if (!hit)
            {
                // Enable enemy field for shooting
                m_gameScreen->getPlayer2Field()->enableUnshotCells();
            }
            break;
        }
        
        case MessageType::ShotResult:
        {
            auto result = ShotResultMessage::fromJson(message.payload);
            // Result of our shot on enemy field
            BattleField* enemyField = m_gameScreen->getPlayer2Field(); // Enemy is always on right
            
            // Track enemy field state
            int idx = result.row * 10 + result.col;
            if (result.hit)
            {
                enemyField->markHit(result.row, result.col);
                m_enemyFieldState[idx] = result.destroyed ? 
                    SeaBattle::CellState::Destroyed : SeaBattle::CellState::Hit;
                // If we hit, we can shoot again
                enemyField->enableUnshotCells();
            }
            else
            {
                enemyField->markMiss(result.row, result.col);
                m_enemyFieldState[idx] = SeaBattle::CellState::Miss;
                // If we missed, wait for opponent's turn
                enemyField->disableAllCells();
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
    
    // Initialize enemy field state
    m_enemyFieldState.fill(SeaBattle::CellState::Empty);
    
    // Create network game model
    m_networkGameModel = std::make_unique<SeaBattle::NetworkGameModel>();
    
    // Send game start message
    m_networkAdapter->sendMessage(
        SeaBattle::Network::createGameStartMessage(isServer));
    
    // Start the game
    m_stackedWidget->setCurrentWidget(m_gameScreen);
    m_gameScreen->getPlayer1Field()->clearAll();
    m_gameScreen->getPlayer2Field()->clearAll();
    
    // Display our ships on the left field
    BattleField* myField = m_gameScreen->getPlayer1Field();
    for (const auto& ship : m_networkGameModel->getMyShips())
    {
        for (const auto& pos : ship.positions)
        {
            myField->markShip(pos.first, pos.second);
        }
    }
    
    // In network mode, always disable our own field (can't shoot ourselves)
    myField->disableAllCells();
    
    m_gameScreen->setExitButtonVisible(true);
    
    // Server always starts first
    if (isServer)
    {
        m_gameScreen->getPlayer2Field()->enableUnshotCells();
    }
    else
    {
        // Client waits for server's first move
        m_gameScreen->getPlayer2Field()->disableAllCells();
    }
}

void MainWindow::cleanupNetwork()
{
    if (m_networkAdapter)
    {
        m_networkAdapter->stop();
        m_networkAdapter.reset();
    }
    m_networkGameModel.reset();
    m_isNetworkGame = false;
    m_isServer = false;
}
