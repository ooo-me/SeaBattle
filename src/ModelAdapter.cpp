#include "ModelAdapter.h"


GameModelAdapter::GameModelAdapter()
    : model(std::make_unique<SeaBattle::GameModel>())
    , sourceType(SeaBattle::ActionSourceFactory::SourceType::Local)
{
    initializeActionSource();
}

GameModelAdapter::GameModelAdapter(SeaBattle::ActionSourceFactory::SourceType sourceType)
    : model(std::make_unique<SeaBattle::GameModel>())
    , sourceType(sourceType)
{
    initializeActionSource();
}

void GameModelAdapter::initializeActionSource()
{
    actionSource = SeaBattle::ActionSourceFactory::create(sourceType, model.get());

    // Set up action source callbacks
    actionSource->setShotCallback([this](int player, const SeaBattle::ShotAction& action) {
        // Shot action event - can be used for logging or network sync
    });

    actionSource->setResultCallback([this](int player, const SeaBattle::ShotResult& result) {
        // Notify about cell update
        if (cellUpdateCallback)
        {
            cellUpdateCallback(player, result.row, result.col, result.resultState);
        }

        // Check for game over
        if (result.gameOver && gameOverCallback)
        {
            gameOverCallback(result.winner);
        }
    });

    actionSource->setErrorCallback([this](const std::string& error) {
        if (errorCallback)
        {
            errorCallback(error);
        }
    });

    actionSource->setPlayerSwitchCallback([this](int newPlayer) {
        if (playerSwitchCallback)
        {
            playerSwitchCallback(newPlayer);
        }
    });

    actionSource->initialize();
}

void GameModelAdapter::startGame()
{
    // Always start with a fresh model and new ship placement
    model = std::make_unique<SeaBattle::GameModel>();
    initializeActionSource();
    model->startGame();
    if (gameStateCallback)
    {
        gameStateCallback(SeaBattle::GameState::Playing);
    }
    if (playerSwitchCallback)
    {
        playerSwitchCallback(model->getCurrentPlayer());
    }
}

bool GameModelAdapter::processShot(int row, int col)
{
    int currentPlayer = model->getCurrentPlayer();

    // Use action source to process the shot
    // All callbacks (cell update, game over, player switch) are handled by the action source
    bool result = actionSource->processShot(currentPlayer, row, col);

    return result;
}

// Методы для получения состояния для GUI
SeaBattle::CellState GameModelAdapter::getPlayerCellState(int player, int row, int col) const
{
    return model->getPlayerCellState(player, row, col);
}

SeaBattle::CellState GameModelAdapter::getEnemyCellState(int player, int row, int col) const
{
    return model->getEnemyViewCellState(player, row, col);
}

const std::vector<SeaBattle::Ship>& GameModelAdapter::getPlayerShips(int player) const
{
    return model->getPlayerShips(player);
}
