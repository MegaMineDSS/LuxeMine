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
    void setRoleAndUserRole(const QString &role);

    QString userRole;

private slots:
    void onTableRightClick(const QPoint &pos);

    void openJobSheet(const QString &jobNo);

    void printJobSheet(const QString &jobNo);


private:
    void show_order_list_with_role(const QString &role, int editableStatusCol);
    void populateCommonOrderRow(int row, const QVariantList &order);
    void hideIrrelevantColumns(const QString &role);
    QStringList getStatusOptions(const QString &role);
    bool shouldShowRow(const QString &role, const QVariantList &order);
    void setupStatusCombo(int row, int col, const QString &role, const QString &currentStatus,
                          const QString &jobNo, const QVariantList &order, const int &editableStatusCol);

    Ui::OrderList *ui;
    LoginWindow *loginWindow = nullptr;


    QString userId;
    QString userName;
};

#endif // ORDERLIST_H
