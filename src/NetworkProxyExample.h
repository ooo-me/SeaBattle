#pragma once

#include "ModelAdapter.h"
#include "LocalGameModelSource.h"
#include "NetworkGameModelSource.h"

// Пример использования сетевого прокси/адаптера
// 
// Этот файл демонстрирует, как использовать новую архитектуру
// для переключения между локальной и сетевой игрой

namespace SeaBattle
{
    // Пример 1: Создание адаптера с локальным источником (по умолчанию)
    inline std::unique_ptr<GameModelAdapter> createLocalGameAdapter()
    {
        // Адаптер автоматически использует локальный источник
        return std::make_unique<GameModelAdapter>();
    }

    // Пример 2: Создание адаптера с явным локальным источником
    inline std::unique_ptr<GameModelAdapter> createExplicitLocalGameAdapter()
    {
        auto localSource = std::make_unique<LocalGameModelSource>();
        return std::make_unique<GameModelAdapter>(std::move(localSource));
    }

    // Пример 3: Создание адаптера с сетевым источником
    inline std::unique_ptr<GameModelAdapter> createNetworkGameAdapter()
    {
        // В будущем здесь можно передать параметры подключения
        // например: host, port, player_id и т.д.
        auto networkSource = std::make_unique<NetworkGameModelSource>();
        return std::make_unique<GameModelAdapter>(std::move(networkSource));
    }

    // Пример 4: Переключение между локальным и сетевым режимом в runtime
    inline void switchToNetworkMode(GameModelAdapter& adapter)
    {
        auto networkSource = std::make_unique<NetworkGameModelSource>();
        adapter.setModelSource(std::move(networkSource));
    }

    inline void switchToLocalMode(GameModelAdapter& adapter)
    {
        auto localSource = std::make_unique<LocalGameModelSource>();
        adapter.setModelSource(std::move(localSource));
    }

    // Пример 5: Использование с callback'ами
    inline void setupGameCallbacks(GameModelAdapter& adapter)
    {
        // Настраиваем обработчики событий
        adapter.setCellUpdateCallback([](int player, int row, int col, CellState state) {
            // Обработка обновления клетки
            // Этот callback будет вызван независимо от источника (локальный или сетевой)
        });

        adapter.setPlayerSwitchCallback([](int newPlayer) {
            // Обработка смены игрока
            // Работает одинаково для локального и сетевого режима
        });

        adapter.setGameOverCallback([](int winner) {
            // Обработка завершения игры
            // Источник (локальный/сетевой) прозрачен для этого callback'а
        });
    }

    // Пример 6: Демонстрация прозрачности источника
    inline void playGameTransparently(GameModelAdapter& adapter)
    {
        // Этот код работает одинаково для локального и сетевого источника
        adapter.startGame();
        
        // Делаем выстрел - источник автоматически определит,
        // нужно ли обработать его локально или отправить по сети
        bool hit = adapter.processShot(0, 0);
        
        // Получаем состояние - источник вернет данные
        // из локальной модели или из сетевого кэша
        int currentPlayer = adapter.getCurrentPlayer();
        CellState state = adapter.getEnemyCellState(currentPlayer, 0, 0);
        
        // Бизнес-логика полностью изолирована от транспорта!
    }

} // namespace SeaBattle
