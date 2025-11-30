#include "BattleField.h"

BattleField::BattleField(bool showShips, QWidget* parent)
    : QWidget(parent), m_showShips(showShips)
{
    QGridLayout* grid = new QGridLayout(this);
    // Без зазоров между ячейками и без внешних отступов
    grid->setSpacing(0);
    grid->setHorizontalSpacing(0);
    grid->setVerticalSpacing(0);
    grid->setContentsMargins(0, 0, 0, 0);

    // Создаем буквенные обозначения для столбцов
    QStringList letters = { "А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "К" };

    // Добавляем пустой угол (оформление заголовков)
    QLabel* corner = new QLabel("");
    corner->setAlignment(Qt::AlignCenter);
    corner->setMinimumSize(30, 30);
    corner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    corner->setStyleSheet("font-size: 14px; font-weight: bold; color: white; background-color: #1F2A44;");
    grid->addWidget(corner, 0, 0);

    // Добавляем буквенные заголовки (выровнены по центру и читаемые)
    for (int col = 0; col < 10; ++col)
    {
        QLabel* label = new QLabel(letters[col]);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(30, 30);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        label->setStyleSheet("font-size: 14px; font-weight: bold; color: white; background-color: #1F2A44;");
        grid->addWidget(label, 0, col + 1);
        grid->setColumnStretch(col + 1, 1);
    }

    m_cells.resize(10);
    for (int row = 0; row < 10; ++row)
    {
        m_cells[row].resize(10);

        QLabel* numberLabel = new QLabel(QString::number(row + 1));
        numberLabel->setAlignment(Qt::AlignCenter);
        numberLabel->setMinimumSize(30, 30);
        numberLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        numberLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: white; background-color: #1F2A44;");
        grid->addWidget(numberLabel, row + 1, 0);
        grid->setRowStretch(row + 1, 1);

        for (int col = 0; col < 10; ++col)
        {
            QPushButton* cell = new QPushButton();
            cell->setMinimumSize(30, 30);
            cell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            cell->setProperty("row", row);
            cell->setProperty("col", col);

            cell->setStyleSheet(
                "QPushButton {"
                "    background-color: #87CEEB;"
                "    border: 1px solid #4682B4;"
                "}"
                "QPushButton:hover {"
                "    background-color: #B0E0E6;"
                "}");

            grid->addWidget(cell, row + 1, col + 1);
            m_cells[row][col] = cell;
            connect(cell, &QPushButton::clicked, this, &BattleField::onCellClicked);
        }
    }
}

static inline bool isShotStyle(const QString& style)
{
    return style.contains("#FF6B6B") || style.contains("#FFFFFF") || style.contains("#FF4757") || style.contains("#CCCCCC");
}

void BattleField::markHit(int row, int col)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 10 && m_cells[row][col])
    {
        QPushButton* cell = m_cells[row][col];
        cell->setStyleSheet(
            "QPushButton {"
            "    background-color: #FF6B6B;"
            "    border: 1px solid #FF4757;"
            "}");
        cell->setEnabled(false);
    }
}

void BattleField::markMiss(int row, int col)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 10 && m_cells[row][col])
    {
        QPushButton* cell = m_cells[row][col];
        cell->setStyleSheet(
            "QPushButton {"
            "    background-color: #FFFFFF;"
            "    border: 1px solid #CCCCCC;"
            "}");
        cell->setEnabled(false);
    }
}

void BattleField::markShip(int row, int col)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 10 && m_cells[row][col])
    {
        QPushButton* cell = m_cells[row][col];
        if (!isShotStyle(cell->styleSheet()))
        {
            cell->setStyleSheet(
                "QPushButton {"
                "    background-color: #2E8B57;"
                "    border: 1px solid #228B22;"
                "}");
        }
    }
}

void BattleField::markDebug(int row, int col)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 10 && m_cells[row][col])
    {
        QPushButton* cell = m_cells[row][col];
        if (!isShotStyle(cell->styleSheet()))
        {
            cell->setStyleSheet(
                "QPushButton {"
                "    background-color: #FFD700;"
                "    border: 2px dashed #B8860B;"
                "}");
        }
    }
}

void BattleField::resetUnfiredCellsStyle()
{
    for (int r = 0; r < 10; ++r)
    {
        for (int c = 0; c < 10; ++c)
        {
            QPushButton* cell = m_cells[r][c];
            if (!cell) continue;
            if (!isShotStyle(cell->styleSheet()))
            {
                cell->setStyleSheet(
                    "QPushButton {"
                    "    background-color: #87CEEB;"
                    "    border: 1px solid #4682B4;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: #B0E0E6;"
                    "}");
            }
        }
    }
}

void BattleField::clearAll()
{
    for (int r = 0; r < 10; ++r)
    {
        for (int c = 0; c < 10; ++c)
        {
            QPushButton* cell = m_cells[r][c];
            if (!cell) continue;
            cell->setStyleSheet(
                "QPushButton {"
                "    background-color: #87CEEB;"
                "    border: 1px solid #4682B4;"
                "}"
                "QPushButton:hover {"
                "    background-color: #B0E0E6;"
                "}");
            cell->setEnabled(true);
        }
    }
}

void BattleField::enableUnshotCells()
{
    for (int r = 0; r < 10; ++r)
    {
        for (int c = 0; c < 10; ++c)
        {
            QPushButton* cell = m_cells[r][c];
            if (!cell) continue;
            if (!isShotStyle(cell->styleSheet()))
            {
                cell->setEnabled(true);
            }
        }
    }
}

void BattleField::setCellEnabled(int row, int col, bool enabled)
{
    if (row >= 0 && row < 10 && col >= 0 && col < 10 && m_cells[row][col])
    {
        m_cells[row][col]->setEnabled(enabled);
    }
}

void BattleField::disableAllCells()
{
    for (int row = 0; row < 10; ++row)
    {
        for (int col = 0; col < 10; ++col)
        {
            if (m_cells[row][col])
            {
                m_cells[row][col]->setEnabled(false);
            }
        }
    }
}

void BattleField::enableAllCells()
{
    for (int row = 0; row < 10; ++row)
    {
        for (int col = 0; col < 10; ++col)
        {
            if (m_cells[row][col])
            {
                m_cells[row][col]->setEnabled(true);
            }
        }
    }
}

void BattleField::setEnabled(bool enabled)
{
    for (int row = 0; row < 10; ++row)
    {
        for (int col = 0; col < 10; ++col)
        {
            if (m_cells[row][col])
            {
                m_cells[row][col]->setEnabled(enabled);
            }
        }
    }
}

void BattleField::onCellClicked()
{
    QPushButton* cell = qobject_cast<QPushButton*>(sender());
    if (cell)
    {
        int row = cell->property("row").toInt();
        int col = cell->property("col").toInt();
        emit cellClicked(row, col);
    }
}
