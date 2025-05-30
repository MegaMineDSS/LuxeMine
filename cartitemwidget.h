#ifndef CARTITEMWIDGET_H
#define CARTITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
// #include <QSpinBox>
// #include <QHBoxLayout>

class CartItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CartItemWidget(int imageId, const QString &goldType, int itemCount, const QPixmap &image, QWidget *parent = nullptr);
    int getImageId() const { return imageId; }
    QString getGoldType() const { return goldType; }
    int getItemCount() const { return itemCount; }
    QPixmap getImage() const {return image; }

signals:
    void quantityChanged(int imageId, const QString &goldType, int newQuantity);
    void removeRequested(int imageId, const QString &goldType);

private slots:
    void onIncrementClicked();
    void onDecrementClicked();
    void onRemoveClicked();

private:
    int imageId;
    QString goldType;
    int itemCount;
    QPixmap image;
    QLabel *imageLabel;
    QLabel *detailsLabel;
    QLabel *quantityLabel;
    QPushButton *incrementButton;
    QPushButton *decrementButton;
    QPushButton *removeButton;
};

#endif // CARTITEMWIDGET_H
