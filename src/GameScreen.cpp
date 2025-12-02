#include "GameScreen.h"

#include "BattleField.h"
#include "Model.h"

GameScreen::GameScreen(QWidget* parent)
    : QWidget(parent)
{
    m_mainLayout = new QHBoxLayout(this);

    m_leftLayout = new QVBoxLayout();
    m_rightLayout = new QVBoxLayout();

    m_player1Label = new QLabel("Ваше поле");
    m_player1Label->setAlignment(Qt::AlignCenter);
    m_player1Label->setStyleSheet("font-size: 18px; font-weight: bold; color: white;");

    m_player2Label = new QLabel("Поле противника");
    m_player2Label->setAlignment(Qt::AlignCenter);
    m_player2Label->setStyleSheet("font-size: 18px; font-weight: bold; color: white;");

    m_player1Field = new BattleField(true);
    m_player2Field = new BattleField(false);

    // Создаём кнопку выхода из игры
    m_exitButton = new QPushButton("Выход из игры");
    m_exitButton->setStyleSheet("font-size: 14px; padding: 10px;");
    m_exitButton->setVisible(false); // Изначально скрыта

    m_mainLayout->addLayout(m_leftLayout);
    m_mainLayout->addLayout(m_rightLayout);

    m_currentPlayer = 0;
    rebuildLayoutsForCurrentPlayer(); // Кнопка выхода будет добавлена в rebuildLayoutsForCurrentPlayer()

    connect(m_player2Field, &BattleField::cellClicked, this, &GameScreen::onEnemyCellClicked);
    connect(m_player1Field, &BattleField::cellClicked, this, &GameScreen::onEnemyCellClicked);
    connect(m_exitButton, &QPushButton::clicked, this, &GameScreen::onExitButtonClicked);
}

void GameScreen::rebuildLayoutsForCurrentPlayer()
{
    auto clearLayout = [](QLayout* layout) {
        while (layout->count() > 0)
        {
            QLayoutItem* item = layout->takeAt(0);
            if (item->widget())
            {
                layout->removeWidget(item->widget());
            }
            delete item;
        }
        };

    clearLayout(m_leftLayout);
    clearLayout(m_rightLayout);

    // Сбрасываем только незастреленные (не трогаем попадания)
    m_player1Field->resetUnfiredCellsStyle();
    m_player2Field->resetUnfiredCellsStyle();

    if (m_currentPlayer == 0)
    {
        m_player1Label->setText("Ваше поле");
        m_player2Label->setText("Поле противника");

        m_leftLayout->addWidget(m_player1Label);
        m_leftLayout->addWidget(m_player1Field);

        m_rightLayout->addWidget(m_player2Label);
        m_rightLayout->addWidget(m_player2Field);

        // Разрешаем выстрелы только по полю противника
        m_player2Field->enableUnshotCells();
        m_player1Field->disableAllCells();
    }
    else
    {
        m_player2Label->setText("Ваше поле");
        m_player1Label->setText("Поле противника");

        m_leftLayout->addWidget(m_player2Label);
        m_leftLayout->addWidget(m_player2Field);

        m_rightLayout->addWidget(m_player1Label);
        m_rightLayout->addWidget(m_player1Field);

        m_player1Field->enableUnshotCells();
        m_player2Field->disableAllCells();
    }
    
    // Добавляем кнопку выхода обратно в правый layout после перестройки
    m_rightLayout->addWidget(m_exitButton, 0, Qt::AlignRight);
}

void GameScreen::onPlayerSwitched(int newPlayer)
{
    m_currentPlayer = newPlayer;
    rebuildLayoutsForCurrentPlayer();
}

void GameScreen::onCellUpdated(int player, int row, int col, SeaBattle::CellState state)
{
    BattleField* targetField = nullptr;
    if (player == 0)
        targetField = m_player2Field;
    else
        targetField = m_player1Field;

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

    // После попадания (ход продолжится) – разрешаем оставшиеся незастреленные клетки этого же поля
    int current = m_currentPlayer;
    if (player == current)
    {
        if (state == SeaBattle::CellState::Hit || state == SeaBattle::CellState::Destroyed)
        {
            // Ход сохраняется, разблокируем оставшиеся незастреленные клетки поля противника
            if (current == 0)
                m_player2Field->enableUnshotCells();
            else
                m_player1Field->enableUnshotCells();
        }
    }
}

void GameScreen::onGameOver(int winner)
{
    m_player1Field->disableAllCells();
    m_player2Field->disableAllCells();

    QMessageBox msgBox;
    msgBox.setWindowTitle("Игра окончена");
    msgBox.setText(QString("Победил игрок %1!").arg(winner + 1));
    QPushButton* newGameButton = msgBox.addButton("Новая игра", QMessageBox::AcceptRole);
    QPushButton* exitButton = msgBox.addButton("Выход", QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.clickedButton() == newGameButton)
        emit returnToMainMenu();
    else if (msgBox.clickedButton() == exitButton)
        qApp->quit();
}

void GameScreen::onEnemyCellClicked(int row, int col)
{
    // Не блокируем все поле: пусть модель решает исход.
    // Клик по клетке будет визуально зафиксирован через markHit/markMiss.
    emit cellClicked(m_currentPlayer, row, col);
}

void GameScreen::onExitButtonClicked()
{
    // Завершаем текущую игру и возвращаемся на экран приветствия
    emit exitGameRequested();
}

void GameScreen::setExitButtonVisible(bool visible)
{
    // Устанавливаем видимость кнопки выхода
    m_exitButton->setVisible(visible);
}
