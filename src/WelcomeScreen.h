#pragma once

#include <QWidget>

class WelcomeScreen : public QWidget
{
    Q_OBJECT
public:
    WelcomeScreen(QWidget* parent = nullptr);

signals:
    void startGameRequested();
    void createServerRequested();
    void joinGameRequested();
};
