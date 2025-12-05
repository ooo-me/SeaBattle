#include "WelcomeScreen.h"

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Морской Бой");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; margin: 50px; color: white;");

    QPushButton* startButton = new QPushButton("Начать игру (локально)");
    QPushButton* createGameButton = new QPushButton("Создать сетевую игру");
    QPushButton* joinGameButton = new QPushButton("Присоединиться к игре");
    QPushButton* exitButton = new QPushButton("Выход");

    QString greenButtonStyle = 
        "QPushButton {"
        "    font-size: 18px;"
        "    padding: 15px 30px;"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}";

    QString blueButtonStyle = 
        "QPushButton {"
        "    font-size: 18px;"
        "    padding: 15px 30px;"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0b7dda;"
        "}";

    QString orangeButtonStyle = 
        "QPushButton {"
        "    font-size: 18px;"
        "    padding: 15px 30px;"
        "    background-color: #FF9800;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e68900;"
        "}";

    QString redButtonStyle = 
        "QPushButton {"
        "    font-size: 18px;"
        "    padding: 15px 30px;"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #da190b;"
        "}";

    startButton->setStyleSheet(greenButtonStyle);
    createGameButton->setStyleSheet(blueButtonStyle);
    joinGameButton->setStyleSheet(orangeButtonStyle);
    exitButton->setStyleSheet(redButtonStyle);

    layout->addWidget(title);
    layout->addWidget(startButton);
    layout->addWidget(createGameButton);
    layout->addWidget(joinGameButton);
    layout->addWidget(exitButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(startButton, &QPushButton::clicked, this, &WelcomeScreen::startGameRequested);
    connect(createGameButton, &QPushButton::clicked, this, &WelcomeScreen::onCreateGameClicked);
    connect(joinGameButton, &QPushButton::clicked, this, &WelcomeScreen::onJoinGameClicked);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

void WelcomeScreen::onCreateGameClicked()
{
    bool ok;
    int port = QInputDialog::getInt(this, "Создать сетевую игру", 
                                    "Введите порт для ожидания подключения:", 
                                    12345, 1024, 65535, 1, &ok);
    if (ok)
    {
        emit createNetworkGameRequested(port);
    }
}

void WelcomeScreen::onJoinGameClicked()
{
    bool ok;
    QString host = QInputDialog::getText(this, "Присоединиться к игре",
                                         "Введите IP-адрес хоста:",
                                         QLineEdit::Normal, "127.0.0.1", &ok);
    if (ok && !host.isEmpty())
    {
        int port = QInputDialog::getInt(this, "Присоединиться к игре",
                                        "Введите порт:",
                                        12345, 1024, 65535, 1, &ok);
        if (ok)
        {
            emit joinNetworkGameRequested(host, port);
        }
    }
}
