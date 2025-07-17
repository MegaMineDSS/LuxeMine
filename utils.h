#ifndef UTILS_H
#define UTILS_H

#include <QWidget>
#include <QScreen>
#include <QGuiApplication>

inline void setWindowSize(QWidget *window) {
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    window->resize(screenWidth * 0.8, screenHeight * 0.8);  // 80% of screen size
    window->show();
}

#endif // UTILS_H
