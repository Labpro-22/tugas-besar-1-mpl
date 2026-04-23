#include <QApplication>

#include "views/GameWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");

    GameWindow window;
    window.setWindowTitle(QStringLiteral("Nimonspoli Board"));
    window.show();

    return app.exec();
}
