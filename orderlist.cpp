#include "orderlist.h"
#include "ui_orderlist.h"

OrderList::OrderList(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderList)
{
    ui->setupUi(this);
}

OrderList::~OrderList()
{
    delete ui;
}
