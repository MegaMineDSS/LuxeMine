#include "orderlist.h"
#include "ui_orderlist.h"
#include <QComboBox>

OrderList::OrderList(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderList)
{
    ui->setupUi(this);

    // Setup column headers
    ui->orderListTableWidget->setColumnCount(6);
    QStringList headers = {"Sr No", "User ID", "Party ID", "Designer Status", "Manufacturer Status", "Accountant Status"};
    ui->orderListTableWidget->setHorizontalHeaderLabels(headers);

    // Insert 1 dummy row
    ui->orderListTableWidget->setRowCount(1);
    ui->orderListTableWidget->setItem(0, 0, new QTableWidgetItem("1"));        // Sr No
    ui->orderListTableWidget->setItem(0, 1, new QTableWidgetItem("sel1"));     // User ID
    ui->orderListTableWidget->setItem(0, 2, new QTableWidgetItem("party1"));   // Party ID

    // Create 3 status combo boxes
    QStringList statusOptions = {"Pending", "Working", "Completed"};

    for (int col = 3; col <= 5; ++col) {
        QComboBox* combo = new QComboBox();
        combo->addItems({"Pending", "Working", "Completed"});
        combo->setCurrentText("Pending");

        // Helper function to set color using both stylesheet and palette
        auto applyColor = [combo](const QString& status) {
            QString color;
            if (status == "Pending") color = "#ffcccc";
            else if (status == "Working") color = "#fff5ba";
            else if (status == "Completed") color = "#ccffcc";

            // Set stylesheet
            combo->setStyleSheet("QComboBox { background-color: " + color + "; }");

            // Force background color via palette (for fallback)
            QPalette palette = combo->palette();
            palette.setColor(QPalette::Base, QColor(color));
            palette.setColor(QPalette::Button, QColor(color));
            combo->setPalette(palette);
        };

        applyColor("Pending");

        connect(combo, &QComboBox::currentTextChanged, applyColor);

        ui->orderListTableWidget->setCellWidget(0, col, combo);
    }



}

OrderList::~OrderList()
{
    delete ui;
}
