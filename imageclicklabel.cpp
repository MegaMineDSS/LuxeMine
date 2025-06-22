#include "imageclicklabel.h"

ImageClickLabel::ImageClickLabel(QWidget *parent)
    : QLabel(parent) {}

void ImageClickLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        emit rightClicked();
    } else {
        QLabel::mousePressEvent(event);  // pass other events
    }
}
