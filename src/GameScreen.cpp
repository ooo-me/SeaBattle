#include "GameScreen.h"

#include "BattleField.h"
#include "IModel.h"

GameScreen::GameScreen(QWidget* parent)
    : QWidget(parent)
{
    // Создаём главный вертикальный layout
    m_mainLayout = new QVBoxLayout(this);

    // Создаём горизонтальный layout для игровых полей
    m_fieldsLayout = new QHBoxLayout();
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

    // Собираем layout для игровых полей
    m_fieldsLayout->addLayout(m_leftLayout);
    m_fieldsLayout->addLayout(m_rightLayout);

    // Создаём горизонтальный layout для кнопок внизу
    m_buttonsLayout = new QHBoxLayout();
    m_buttonsLayout->addStretch(); // Растягивающий элемент слева для выравнивания кнопки вправо
    m_buttonsLayout->addWidget(m_exitButton);

    // Создаём контейнер для layout кнопок
    m_buttonsWidget = new QWidget(this); // Родитель this для автоматического управления памятью
    m_buttonsWidget->setLayout(m_buttonsLayout);
    // Используем минимальную высоту вместо фиксированной, чтобы текст помещался полностью
    m_buttonsWidget->setMinimumHeight(60);

    // Добавляем все в главный layout
    m_mainLayout->addLayout(m_fieldsLayout);
    m_mainLayout->addWidget(m_buttonsWidget);

    m_currentPlayer = 0;
    m_localPlayer = 0;
    rebuildLayoutsForCurrentPlayer();

    connect(m_player2Field, &BattleField::cellClicked, this, &GameScreen::onEnemyCellClicked);
    connect(m_player1Field, &BattleField::cellClicked, this, &GameScreen::onEnemyCellClicked);
    connect(m_exitButton, &QPushButton::clicked, this, &GameScreen::onExitButtonClicked);
}

void GameScreen::setLocalPlayer(int localPlayer)
{
    m_localPlayer = localPlayer;
    // Не вызываем rebuildLayoutsForCurrentPlayer здесь,
    // так как m_currentPlayer ещё может быть не установлен.
    // rebuildLayoutsForCurrentPlayer будет вызван через onPlayerSwitched.
}

void GameScreen::setPlayerNames(const QString& localName, const QString& opponentName)
{
    m_localPlayerName = localName;
    m_opponentName = opponentName;
    updateLabels();
}

void GameScreen::updateLabels()
{
    if (m_localPlayerName.isEmpty())
    {
        m_player1Label->setText("Ваше поле");
        m_player2Label->setText("Поле противника");
    }
    else
    {
        m_player1Label->setText(QString("Ваше поле (%1)").arg(m_localPlayerName));
        m_player2Label->setText(QString("Поле противника (%1)").arg(m_opponentName));
    }
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

    // m_player1Field - всегда "Ваше поле" (поле локального игрока)
    // m_player2Field - всегда "Поле противника" (куда мы стреляем)
    updateLabels();

    m_leftLayout->addWidget(m_player1Label);
    m_leftLayout->addWidget(m_player1Field);

    m_rightLayout->addWidget(m_player2Label);
    m_rightLayout->addWidget(m_player2Field);

    // Разрешаем выстрелы только если сейчас ход локального игрока
    if (m_currentPlayer == m_localPlayer)
    {
        m_player2Field->enableUnshotCells();
        m_player1Field->disableAllCells();
    }
    else
    {
        m_player1Field->disableAllCells();
        m_player2Field->disableAllCells();
    }
}

void GameScreen::onPlayerSwitched(int newPlayer)
{
    m_currentPlayer = newPlayer;
    rebuildLayoutsForCurrentPlayer();
}

void GameScreen::onCellUpdated(int player, int row, int col, SeaBattle::CellState state)
{
    // player - это тот кто стрелял
    // Если стрелял локальный игрок - обновляем поле противника
    BattleField* targetField = nullptr;
    if (player == m_localPlayer)
        targetField = m_player2Field;  // Поле противника
    else
        targetField = m_player1Field;  // Наше поле (противник в нас попал)

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

    // После попадания (ход продолжится) – разрешаем оставшиеся незастреленные клетки поля противника
    if (player == m_localPlayer)
    {
        if (state == SeaBattle::CellState::Hit || state == SeaBattle::CellState::Destroyed)
        {
            m_player2Field->enableUnshotCells();
        }
    }
}

void GameScreen::onGameOver(bool win)
{
    m_player1Field->disableAllCells();
    m_player2Field->disableAllCells();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Игра окончена");
    msgBox.setText(win ? QString("Вы выйграли!") : QString("Вы проиграли!"));
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
    // Передаём локального игрока, так как именно он делает выстрел
    emit cellClicked(m_localPlayer, row, col);
}

void GameScreen::onExitButtonClicked()
{
    // Показываем диалог подтверждения выхода
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Подтверждение выхода");
    msgBox.setText("Вы уверены, что хотите выйти из игры? Текущий прогресс будет потерян.");
    msgBox.addButton("Вернуться к игре", QMessageBox::RejectRole);
    QPushButton* exitButton = msgBox.addButton("Выйти", QMessageBox::AcceptRole);

    msgBox.exec();

    // Если пользователь подтвердил выход, завершаем текущую игру и возвращаемся на экран приветствия
    if (msgBox.clickedButton() == exitButton)
    {
        emit exitGameRequested();
    }
    // Если пользователь нажал "Вернуться к игре", ничего не делаем - игра продолжается
}

void GameScreen::setExitButtonVisible(bool visible)
{
    // Устанавливаем видимость кнопки выхода
    m_exitButton->setVisible(visible);
}
