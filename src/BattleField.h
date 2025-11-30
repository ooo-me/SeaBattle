#pragma once

#include <QWidget>

class BattleField : public QWidget
{
    Q_OBJECT
public:
    BattleField(bool showShips = false, QWidget* parent = nullptr);

public slots:
    void markHit(int row, int col);
    void markMiss(int row, int col);
    void markShip(int row, int col);
    void markDebug(int row, int col);
    void setCellEnabled(int row, int col, bool enabled);
    void disableAllCells();
    void enableAllCells();
    void setEnabled(bool enabled); // overrides QWidget behavior for all cells
    void resetUnfiredCellsStyle();
    void clearAll();
    void enableUnshotCells(); // включает только клетки доступные для выстрела

signals:
    void cellClicked(int row, int col);

private slots:
    void onCellClicked();

private:
    bool m_showShips;
    QVector<QVector<QPushButton*>> m_cells;
};
