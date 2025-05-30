#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    // Get screen size
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // Set window size to 80% of screen size
    w.resize(screenWidth * 0.8, screenHeight * 0.8);

    w.show();
    return a.exec();
}
