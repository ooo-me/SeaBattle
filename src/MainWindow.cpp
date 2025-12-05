#include "MainWindow.h"

#include "BattleField.h"
#include "GameScreen.h"
#include "ModelAdapter.h"
#include "WelcomeScreen.h"

#include <QStackedWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_connectionDialog(nullptr)
    , m_isNetworkGame(false)
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
    connect(m_welcomeScreen, &WelcomeScreen::createNetworkGameRequested, this, &MainWindow::onCreateNetworkGame);
    connect(m_welcomeScreen, &WelcomeScreen::joinNetworkGameRequested, this, &MainWindow::onJoinNetworkGame);
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
    
    // Сбрасываем флаг сетевой игры
    m_isNetworkGame = false;
}

void MainWindow::onCellClicked(int player, int row, int col)
{
    // Текущий игрок до выстрела
    int before = m_gameModel->getCurrentPlayer();
    if (player != before)
    {
        return;
    }

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
    // В сетевом режиме не показываем сообщение о смене хода
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

void MainWindow::onCreateNetworkGame(int port)
{
    m_isNetworkGame = true;
    showConnectionWaitingDialog(true, QString("Ожидание подключения на порту %1...").arg(port));
    
    // TODO: Здесь будет реализация создания сетевого сервера
    // После успешного подключения:
    // hideConnectionWaitingDialog();
    // showGameScreen();
    
    // Временная заглушка
    QTimer::singleShot(NETWORK_STUB_DELAY_MS, this, &MainWindow::showNetworkModeStub);
}

void MainWindow::onJoinNetworkGame(const QString& host, int port)
{
    m_isNetworkGame = true;
    showConnectionWaitingDialog(false, QString("Подключение к %1:%2...").arg(host).arg(port));
    
    // TODO: Здесь будет реализация подключения к сетевому серверу
    // После успешного подключения:
    // hideConnectionWaitingDialog();
    // showGameScreen();
    
    // Временная заглушка
    QTimer::singleShot(NETWORK_STUB_DELAY_MS, this, &MainWindow::showNetworkModeStub);
}

void MainWindow::showConnectionWaitingDialog(bool isHost, const QString& info)
{
    if (!m_connectionDialog)
    {
        m_connectionDialog = new QProgressDialog(this);
        m_connectionDialog->setWindowTitle("Сетевая игра");
        m_connectionDialog->setWindowModality(Qt::WindowModal);
        m_connectionDialog->setMinimumDuration(0);
        m_connectionDialog->setRange(0, 0); // Indeterminate progress
        
        // Подключаем обработчик отмены один раз при создании
        connect(m_connectionDialog, &QProgressDialog::canceled, this, [this] {
            hideConnectionWaitingDialog();
            m_isNetworkGame = false;
            // TODO: Отменить сетевое подключение
        });
    }
    
    m_connectionDialog->setLabelText(info);
    m_connectionDialog->setCancelButtonText("Отмена");
    m_connectionDialog->show();
}

void MainWindow::hideConnectionWaitingDialog()
{
    if (m_connectionDialog)
    {
        m_connectionDialog->hide();
    }
}

void MainWindow::showNetworkModeStub()
{
    hideConnectionWaitingDialog();
    QMessageBox::information(this, "Сетевой режим", 
                            "Сетевой режим пока в разработке.\nЗапускается локальная игра.");
    m_isNetworkGame = false;
    showGameScreen();
}
