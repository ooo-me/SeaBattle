#pragma once

#include <QObject>
#include <functional>

enum class ConnectionStatus;

// Класс для управления сетевым подключением
class NetworkClient : public QObject
{
    Q_OBJECT
public:
    using ConnectionStatusCallback = std::function<void(ConnectionStatus status)>;

    explicit NetworkClient(QObject* parent = nullptr);
    
    // Подключиться к серверу
    void connectToServer(const QString& host, int port);
    
    // Отключиться от сервера
    void disconnect();
    
    // Получить текущий статус подключения
    ConnectionStatus getStatus() const { return m_currentStatus; }
    
    // Установить callback для изменения статуса
    void setStatusCallback(ConnectionStatusCallback callback) { m_statusCallback = callback; }
    
signals:
    void connectionStatusChanged(ConnectionStatus status);

private:
    void setStatus(ConnectionStatus status);
    
    ConnectionStatus m_currentStatus;
    ConnectionStatusCallback m_statusCallback;
};
