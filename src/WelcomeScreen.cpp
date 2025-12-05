#include "WelcomeScreen.h"

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Морской Бой");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; margin: 50px; color: white;");

    QPushButton* startButton = new QPushButton("Начать игру");
    QPushButton* createServerButton = new QPushButton("Создать игру");
    QPushButton* joinGameButton = new QPushButton("Присоединиться");
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
    createServerButton->setStyleSheet(blueButtonStyle);
    joinGameButton->setStyleSheet(blueButtonStyle);
    exitButton->setStyleSheet(redButtonStyle);

    layout->addWidget(title);
    layout->addWidget(startButton);
    layout->addWidget(createServerButton);
    layout->addWidget(joinGameButton);
    layout->addWidget(exitButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(startButton, &QPushButton::clicked, this, &WelcomeScreen::startGameRequested);
    connect(createServerButton, &QPushButton::clicked, this, &WelcomeScreen::createServerRequested);
    connect(joinGameButton, &QPushButton::clicked, this, &WelcomeScreen::joinGameRequested);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}
