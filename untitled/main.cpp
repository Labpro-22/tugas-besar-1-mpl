#include <QApplication>

#include "GameWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle("Fusion");

    GameWindow w;
    w.setWindowTitle("Nimonspoli Board - Qt");
    w.resize(1420, 940);
    w.show();

    return a.exec();
}
