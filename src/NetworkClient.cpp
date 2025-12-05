#include "NetworkClient.h"
#include "GameScreen.h"

#include <QTimer>

namespace {
    constexpr int CONNECTION_SIMULATION_DELAY_MS = 1000; // Задержка симуляции подключения
}

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , m_currentStatus(ConnectionStatus::Disconnected)
{
}

void NetworkClient::connectToServer(const QString& host, int port)
{
    // Устанавливаем статус "Ожидание"
    setStatus(ConnectionStatus::Waiting);
    
    // Имитация подключения с задержкой
    // В реальной реализации здесь будет сетевой код
    QTimer::singleShot(CONNECTION_SIMULATION_DELAY_MS, this, [this]() {
        // Имитируем успешное подключение
        setStatus(ConnectionStatus::Connected);
    });
}

void NetworkClient::disconnect()
{
    setStatus(ConnectionStatus::Disconnected);
}

void NetworkClient::simulateTimeout()
{
    setStatus(ConnectionStatus::Timeout);
}

void NetworkClient::simulateError()
{
    setStatus(ConnectionStatus::Error);
}

void NetworkClient::simulateDisconnect()
{
    setStatus(ConnectionStatus::Disconnected);
}

void NetworkClient::setStatus(ConnectionStatus status)
{
    if (m_currentStatus != status)
    {
        m_currentStatus = status;
        
        // Уведомляем через callback
        if (m_statusCallback)
        {
            m_statusCallback(status);
        }
        
        // Отправляем сигнал
        emit connectionStatusChanged(status);
    }
}
