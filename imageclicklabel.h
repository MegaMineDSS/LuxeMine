#ifndef IMAGECLICKLABEL_H
#define IMAGECLICKLABEL_H

#include <QLabel>
#include <QMouseEvent>

class ImageClickLabel : public QLabel {
    Q_OBJECT
public:
    explicit ImageClickLabel(QWidget *parent = nullptr);

signals:
    void rightClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // IMAGECLICKLABEL_H
