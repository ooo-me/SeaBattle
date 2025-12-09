#include "MainWindow.h"

#include "BattleField.h"
#include "GameScreen.h"
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
    m_gameScreen = new GameScreen();

    m_stackedWidget->addWidget(m_welcomeScreen);
    m_stackedWidget->addWidget(m_gameScreen);

    // Подключаем сигналы
    connect(m_welcomeScreen, &WelcomeScreen::startGameRequested, this, &MainWindow::showGameScreen);
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

    m_gameModel.StartGame();
    updateBattleFields();
    showTurnMessage(m_gameModel.GetCurrentPlayer());

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
    int before = m_gameModel.GetCurrentPlayer();
    if (player != before)
    {
        return;
    }

    bool hit = m_gameModel.ProcessShot(row, col);

    // Если попадание и игра не окончена — разрешаем ОСТАВШИЕСЯ незастреленные клетки на поле противника
    if (hit && m_gameModel.GetGameState() == SeaBattle::GameState::Playing)
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
    int after = m_gameModel.GetCurrentPlayer();
    if (after == before && m_gameModel.GetGameState() == SeaBattle::GameState::Playing)
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

void MainWindow::onGameOver(int winner)
{
    m_gameScreen->onGameOver(winner);
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

    int current = m_gameModel.GetCurrentPlayer();

    const auto& player1Ships = m_gameModel.GetPlayerShips(0);
    const auto& player2Ships = m_gameModel.GetPlayerShips(1);

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
    QMessageBox msgBox;
    msgBox.setWindowTitle("Смена хода");
    msgBox.setText(QString("Ход игрока %1").arg(player + 1));
    msgBox.addButton("Продолжить", QMessageBox::AcceptRole);
    msgBox.exec();
}
