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

    MainWindow window;
    window.show();

    return app.exec();
}
