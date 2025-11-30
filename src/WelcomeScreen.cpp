#include "WelcomeScreen.h"

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("Морской Бой");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; margin: 50px; color: white;");

    QPushButton* startButton = new QPushButton("Начать игру");
    QPushButton* exitButton = new QPushButton("Выход");

    startButton->setStyleSheet(
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
        "}");

    exitButton->setStyleSheet(
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
        "}");

    layout->addWidget(title);
    layout->addWidget(startButton);
    layout->addWidget(exitButton);
    layout->setAlignment(Qt::AlignCenter);

    connect(startButton, &QPushButton::clicked, this, &WelcomeScreen::startGameRequested);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}
