#pragma once

#include <QWidget>

class WelcomeScreen : public QWidget
{
    Q_OBJECT
public:
    WelcomeScreen(QWidget* parent = nullptr);

signals:
    void startGameRequested();
    void createNetworkGameRequested(int port);
    void joinNetworkGameRequested(const QString& host, int port);

private slots:
    void onCreateGameClicked();
    void onJoinGameClicked();
};
