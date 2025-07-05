#ifndef ORDERLIST_H
#define ORDERLIST_H

#include <QDialog>
#include "loginwindow.h"

namespace Ui {
class OrderList;
}

class OrderList : public QDialog
{
    Q_OBJECT

public:
    explicit OrderList(QWidget *parent = nullptr, const QString& role = "");
    ~OrderList();

    void setRoleAndUserInfo(const QString& role, const QString& userId, const QString& userName);


private slots:
    void onTableRightClick(const QPoint &pos);


private:
    void show_order_list_designer();
    Ui::OrderList *ui;
    LoginWindow *loginWindow = nullptr;

    QString userRole;
    QString userId;
    QString userName;
};

#endif // ORDERLIST_H
