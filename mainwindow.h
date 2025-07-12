#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QPixmap>
#include <QResizeEvent>

#include "addcatalog.h"
#include "admin.h"
#include "loginwindow.h"
#include "orderlist.h"
#include "ordermenu.h"
#include "user.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();
    void on_orderBookButton_clicked();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;


private:
    Ui::MainWindow *ui;
    Admin *newAdmin = nullptr;
    User *newUser = nullptr;
    AddCatalog *newAddCatalog = nullptr;
    OrderMenu *newOrderMenu = nullptr;
    LoginWindow *loginWindow = nullptr;
    OrderList *newOrderList = nullptr;

    QPixmap currentBackground;

    void setRandomBackground();
    void updateBackground();

};

#endif // MAINWINDOW_H
