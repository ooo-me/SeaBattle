# Клиент SeaBattle - Документация

## Обзор

Реализован клиент для сетевой игры SeaBattle с использованием boost::asio и полной системой обработки статусов соединения.

## Архитектура решения

Реализация состоит из трёх основных компонентов:

### 1. Протокольный слой (Protocol.h)
Определяет протокол обмена сообщениями между клиентом и сервером:

- **Типы сообщений**: Connect, Disconnect, GameStart, GameOver, ShootRequest, ShootResponse, Error, Ping/Pong
- **Классы сообщений**: Базовый класс `Message` с конкретными реализациями для каждого типа
- **Сериализация**: Бинарный протокол с заголовком (тип + размер payload) и данными
- **Коды результатов**: Success, InvalidMove, NotYourTurn, GameNotStarted, ServerError

### 2. Сетевой клиент (NetworkClient.h/cpp)
Основной сетевой слой на основе boost::asio:

- **Асинхронные операции**: Неблокирующие connect, send и receive операции
- **Отслеживание статусов соединения**: 
  - `Disconnected`: Начальное состояние, нет соединения
  - `Connecting`: Попытка соединения в процессе
  - `Connected`: Успешно соединён с сервером
  - `Error`: Произошла ошибка соединения
  - `Timeout`: Превышено время ожидания операции
  - `Disconnecting`: Выполняется корректное отключение

- **Возможности**:
  - Автоматическая обработка таймаутов при подключении
  - Очередь сообщений для отправки
  - Система callback'ов для событий
  - Потокобезопасное обновление статусов
  - Обработка ошибок и восстановление

### 3. Игровой сетевой адаптер (GameNetworkAdapter.h/cpp)
Высокоуровневый интерфейс для игры:

- Связывает NetworkClient с игровой логикой
- Преобразует игровые действия в сетевые сообщения
- Управляет жизненным циклом сетевого потока
- Предоставляет игровые callback'и
- Без зависимостей от Qt (только C++20 + boost)

## Использование

### Базовое использование клиента

```cpp
#include "NetworkClient.h"

using namespace SeaBattle::Network;

// Создание клиента
auto client = std::make_shared<NetworkClient>();

// Установка callback'ов
client->setConnectionStatusCallback(
    [](ConnectionStatus status, const std::string& message) {
        std::cout << "Статус: " << connectionStatusToString(status) 
                  << " - " << message << std::endl;
    });

client->setMessageReceivedCallback(
    [](std::unique_ptr<Message> message) {
        // Обработка полученного сообщения
        switch (message->getType()) {
            case MessageType::ShootResponse:
                auto* response = dynamic_cast<ShootResponseMessage*>(message.get());
                // Обработка ответа...
                break;
        }
    });

// Запуск клиента в отдельном потоке
std::thread clientThread([client]() {
    client->run();
});

// Подключение к серверу
client->connectAsync("localhost", 8080, std::chrono::seconds(10));

// Отправка сообщения
if (client->isConnected()) {
    auto shootMsg = std::make_unique<ShootRequestMessage>(5, 5);
    client->sendMessage(std::move(shootMsg));
}

// Завершение работы
client->disconnect();
client->stop();
clientThread.join();
```

### Использование игрового адаптера

```cpp
#include "GameNetworkAdapter.h"

using namespace SeaBattle::Network;

// Создание адаптера
GameNetworkAdapter adapter;

// Установка игровых callback'ов
adapter.setGameActionCallback(
    [](int row, int col, bool hit) {
        std::cout << "Выстрел по (" << row << "," << col << ") - "
                  << (hit ? "ПОПАДАНИЕ!" : "ПРОМАХ") << std::endl;
    });

adapter.setConnectionErrorCallback(
    [](const std::string& error) {
        std::cerr << "Ошибка: " << error << std::endl;
    });

// Подключение к серверу
adapter.connect("localhost", 8080, "ИмяИгрока");

// Отправка игрового действия
adapter.sendShootAction(3, 7);

// Завершение работы
adapter.disconnect();
```

## Спецификация протокола сообщений

### Формат сообщения

Все сообщения следуют бинарному формату:

```
[Заголовок: 5 байт] [Данные: N байт]

Заголовок:
  - Байт 0: MessageType (uint8)
  - Байты 1-4: Размер данных (uint32, big-endian)

Данные:
  - Специфичные для типа сообщения
```

### Типы сообщений

1. **Connect** (Тип: 0)
   - Данные: Имя игрока (UTF-8 строка)
   
2. **Disconnect** (Тип: 2)
   - Данные: Нет
   
3. **GameStart** (Тип: 10)
   - Данные: Конфигурация игры
   
4. **GameOver** (Тип: 11)
   - Данные: Информация о победителе
   
5. **ShootRequest** (Тип: 20)
   - Данные: 2 байта (строка, столбец)
   
6. **ShootResponse** (Тип: 21)
   - Данные: 2 байта (код результата, флаг попадания)
   
7. **Error** (Тип: 30)
   - Данные: Сообщение об ошибке (UTF-8 строка)
   
8. **Ping/Pong** (Типы: 40/41)
   - Данные: Нет

## Поток статусов соединения

```
Disconnected (Отключен)
    |
    | connectAsync()
    v
Connecting (Подключается) --timeout--> Timeout (Таймаут)
    |                                       |
    | успех                                 | disconnect()
    v                                       v
Connected (Подключен) --ошибка--> Error (Ошибка) --> Disconnected
    |                                   |
    | disconnect()                      | disconnect()
    v                                   v
Disconnecting (Отключается) -------> Disconnected
```

## Потокобезопасность

- Все публичные методы `NetworkClient` потокобезопасны
- Callback'и вызываются из потока io_context
- Обновления статусов используют атомарные операции
- Очередь отправки защищена мьютексом

## Обработка ошибок

Клиент обрабатывает различные сценарии ошибок:

1. **Таймаут соединения**: Настраиваемое время ожидания подключения
2. **Сетевые ошибки**: Коды ошибок boost преобразуются в статусы соединения
3. **Обнаружение разрыва**: Обработка EOF и ошибок сброса соединения
4. **Ошибки сообщений**: Исключения сериализации/десериализации
5. **Ошибки отправки**: Управление очередью и callback'и ошибок

## Сборка

### Требования
- Компилятор C++20
- Boost 1.83 или выше (boost::asio, boost::system)
- CMake 3.15+

### Интеграция с проектом

Сетевой клиент интегрирован в основное приложение SeaBattle:

```cmake
find_package(Boost REQUIRED COMPONENTS system)

target_sources(SeaBattle PRIVATE
    NetworkClient.cpp
    NetworkClient.h
    GameNetworkAdapter.cpp
    GameNetworkAdapter.h
    Protocol.h
)

target_link_libraries(SeaBattle PRIVATE
    Boost::system
)
```

## Тестирование

Предоставлены примеры программ, демонстрирующие:
- Управление соединением
- Callback'и статусов
- Отправку сообщений
- Обработку ошибок
- Поведение при таймауте

Файлы примеров:
- `ClientExample.cpp` - базовый пример
- `NetworkClientUsageExample.cpp` - комплексный пример

## Ключевые особенности реализации

1. **Без зависимостей от Qt**: Использует только стандартный C++20 и boost
2. **Асинхронные операции**: Неблокирующие операции для отзывчивого UI
3. **Расширяемый протокол**: Легко добавлять новые типы сообщений
4. **Система статусов**: Изменения статусов триггерят callback'и для обновления UI
5. **Надёжная доставка**: Клиент поддерживает очередь отправки
6. **Обработка таймаутов**: Автоматическое определение зависших соединений
7. **Потокобезопасность**: Корректная работа в многопоточном окружении

## Будущие улучшения

Возможные улучшения в будущих версиях:

1. Поддержка SSL/TLS через boost::asio::ssl
2. Сжатие сообщений
3. Автоматическое переподключение
4. Пул соединений
5. Отслеживание задержки
6. Система подтверждения сообщений
7. Бинарная сериализация через boost::serialization или protobuf
8. Поддержка WebSocket через boost::beast

## Заключение

Реализация полностью соответствует требованиям задачи:

✅ Класс-клиент на boost::asio/beast
✅ Подключение к серверу и обмен командами
✅ Передача игровых действий
✅ Обработка статусов: ожидание, успех, ошибка, таймаут, разрыв
✅ Сериализация/десериализация сообщений по единой схеме
✅ Взаимодействие с игрой через абстракцию
✅ Без Qt, используется C++20 и boost
✅ Прототип клиентской части с системой статусов готов
