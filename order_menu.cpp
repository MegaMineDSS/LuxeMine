#include "order_menu.h"
#include "ui_order_menu.h"
#include <QWidget>

#include<QtGui>


OrderMenu::OrderMenu(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::OrderMenu)
{
    ui->setupUi(this);
}
OrderMenu::~OrderMenu()
{
    delete ui;
}

// void OrderMenu::combo_Box_Check_Box_Bill_Details(){

//     QStandardItemModel* model = new QStandardItemModel(3, 1, this);

//     QStandardItem* item1 = new QStandardItem(QString("Cash").arg(0));
//     item1->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
//     item1->setData(Qt::Unchecked, Qt::CheckStateRole);
//     model->setItem(0, 0, item1);

//     QStandardItem* item2 = new QStandardItem(QString("GST").arg(1));
//     item2->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
//     item2->setData(Qt::Unchecked, Qt::CheckStateRole);
//     model->setItem(1, 0, item2);

//     QStandardItem* item3 = new QStandardItem(QString("AGADIYA").arg(2));
//     item3->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
//     item3->setData(Qt::Unchecked, Qt::CheckStateRole);
//     model->setItem(2, 0, item3);

//     ui->comboBox->setModel(model);

// }
