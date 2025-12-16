#pragma once

#include <QWidget>

class WelcomeScreen : public QWidget
{
    Q_OBJECT
public:
    WelcomeScreen(QWidget* parent = nullptr);

signals:
    void startGameRequested(const QString& playerName);

private slots:
    void onStartButtonClicked();
};
