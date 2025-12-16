#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class WaitingScreen : public QWidget
{
    Q_OBJECT
public:
    explicit WaitingScreen(QWidget* parent = nullptr);

public slots:
    void setStatusWaiting();
    void setStatusLoading();

private:
    QLabel* m_statusLabel;
};
