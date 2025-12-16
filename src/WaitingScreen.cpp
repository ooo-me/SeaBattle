#include "WaitingScreen.h"

WaitingScreen::WaitingScreen(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_statusLabel = new QLabel("Ожидание других игроков");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");

    layout->addWidget(m_statusLabel);
    layout->setAlignment(Qt::AlignCenter);
}

void WaitingScreen::setStatusWaiting()
{
    m_statusLabel->setText("Ожидание других игроков");
}

void WaitingScreen::setStatusLoading()
{
    m_statusLabel->setText("Загрузка");
}
