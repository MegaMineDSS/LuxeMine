#ifndef ORDERMENU_H
#define ORDERMENU_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QPixmap>

namespace Ui {
class OrderMenu;
}

class OrderMenu : public QDialog
{
    Q_OBJECT

public:
    void setSellerInfo(const QString &name, const QString &id);
    void insertDummyOrder();

    explicit OrderMenu(QWidget *parent = nullptr);
    ~OrderMenu();

private slots:
    void on_savePushButton_clicked();
    void closeEvent(QCloseEvent *event);
    int getNextJobNumber();
    int getNextOrderNumberForSeller(const QString& sellerId);

private:
    QString selectAndSaveImage(const QString &prefix);
    Ui::OrderMenu *ui;
    QString imagePath1, imagePath2;

    int dummyOrderId = -1;
    bool isSaved = false;
    QString currentSellerId;
    QString currentSellerName;

};

#endif // ORDERMENU_H
