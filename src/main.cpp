#include "RemoteModel.h"
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Устанавливаем стиль приложения
    app.setStyleSheet(
        "QMainWindow {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                               stop:0 #1E3C72, stop:1 #2A5298);"
        "}");

    auto gameModel = RemoteModel();

    MainWindow window(gameModel);
    // Подключаем callback'и к GameScreen
    gameModel.setCellUpdateCallback([&window](int player, int row, int col, SeaBattle::CellState state) {
        QMetaObject::invokeMethod(&window, &MainWindow::onCellUpdated, player, row, col, state);
        });

    gameModel.setPlayerSwitchCallback([&window](int newPlayer) {
        QMetaObject::invokeMethod(&window, &MainWindow::onPlayerSwitched, newPlayer);
        });

    gameModel.setGameOverCallback([&window](int winner) {
        QMetaObject::invokeMethod(&window, &MainWindow::onGameOver, winner);
        });
    window.show();

    return app.exec();
}
