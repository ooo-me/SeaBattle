#pragma once

#include "Model.h"
#include <functional>
#include <memory>

namespace SeaBattle
{
    // Forward declarations
    struct ShotAction
    {
        int row;
        int col;
    };

    struct ShotResult
    {
        int row;
        int col;
        bool hit;
        CellState resultState;
        bool gameOver;
        int winner;
    };

    // Interface for action sources (local or network)
    class IActionSource
    {
    public:
        virtual ~IActionSource() = default;

        // Callbacks for game events
        using ShotCallback = std::function<void(int player, const ShotAction& action)>;
        using ResultCallback = std::function<void(int player, const ShotResult& result)>;
        using ErrorCallback = std::function<void(const std::string& error)>;
        using PlayerSwitchCallback = std::function<void(int newPlayer)>;

        // Set callbacks for events
        virtual void setShotCallback(ShotCallback callback) = 0;
        virtual void setResultCallback(ResultCallback callback) = 0;
        virtual void setErrorCallback(ErrorCallback callback) = 0;
        virtual void setPlayerSwitchCallback(PlayerSwitchCallback callback) = 0;

        // Process a shot action
        virtual bool processShot(int player, int row, int col) = 0;

        // Check if a shot is valid
        virtual bool isValidShot(int player, int row, int col) const = 0;

        // Get current player
        virtual int getCurrentPlayer() const = 0;

        // Initialize/reset the action source
        virtual void initialize() = 0;
    };

    // Factory for creating action sources
    class ActionSourceFactory
    {
    public:
        enum class SourceType
        {
            Local,
            Network
        };

        static std::unique_ptr<IActionSource> create(SourceType type, GameModel* model);
    };

} // namespace SeaBattle
