#include <QApplication>
#include <QCoreApplication>
#include <Qt>

#include "views/GameWindow.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");

    GameWindow window;
    window.setWindowTitle(QStringLiteral("Nimonspoli Board"));
    window.show();

    return app.exec();
}
