#ifndef CARTITEMWIDGET_H
#define CARTITEMWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

class CartItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CartItemWidget(int imageId,
                            const QString &goldType,
                            int itemCount,
                            const QPixmap &image,
                            QWidget *parent = nullptr);

    int getImageId()   const { return imageId; }
    QString getGoldType() const { return goldType; }
    int getQuantity()  const { return itemCount; }
    QPixmap getImage() const { return image; }


    // Updates UI + internal count WITHOUT emitting quantityChanged
    void setQuantity(int qty);

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
    QLabel *imageLabel{nullptr};
    QLabel *detailsLabel{nullptr};
    QLabel *quantityLabel{nullptr};
    QPushButton *incrementButton{nullptr};
    QPushButton *decrementButton{nullptr};
    QPushButton *removeButton{nullptr};
    QHBoxLayout *layout{nullptr};

    void rebuildDetails();
};

#endif // CARTITEMWIDGET_H
