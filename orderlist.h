#ifndef ORDERLIST_H
#define ORDERLIST_H

#include <QDialog>

namespace Ui {
class OrderList;
}

class OrderList : public QDialog
{
    Q_OBJECT

public:
    explicit OrderList(QWidget *parent = nullptr);
    ~OrderList();

private:
    Ui::OrderList *ui;
};

#endif // ORDERLIST_H
