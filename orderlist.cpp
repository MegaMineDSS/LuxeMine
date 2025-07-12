#include "orderlist.h"
#include "ui_orderlist.h"

#include <QComboBox>
#include <DatabaseUtils.h>
#include <QMessageBox>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QMenu>

#include "jobsheet.h"

OrderList::OrderList(QWidget *parent, const QString &role)
    : QDialog(parent), ui(new Ui::OrderList)
{
    ui->setupUi(this);

    ui->orderListTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->orderListTableWidget, &QTableWidget::customContextMenuRequested,
            this, &OrderList::onTableRightClick);

    // qDebug()<<role<<"-------------------------------role";
    if (role == "designer")
    {
        show_order_list_designer();
    }
}

void OrderList::setRoleAndUserInfo(const QString &role, const QString &userIdValue, const QString &userNameValue)
{
    userRole = role;
    userId = userIdValue;
    userName = userNameValue;
}

OrderList::~OrderList()
{
    delete ui;
}

void OrderList::onTableRightClick(const QPoint &pos)
{
    QModelIndex index = ui->orderListTableWidget->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu contextMenu;
    QAction *jobSheetAction = contextMenu.addAction("Show Job Sheet");

    QAction *selectedAction = contextMenu.exec(ui->orderListTableWidget->viewport()->mapToGlobal(pos));
    if (selectedAction == jobSheetAction)
    {
        JobSheet *sheet = new JobSheet(this);
        sheet->exec(); // Just open the dialog
    }
}

void OrderList::show_order_list_designer()
{
    ui->orderListTableWidget->setColumnCount(7); // Including Sr No
    QStringList headers = {"Sr No", "User ID", "Party ID", "Job No", "Designer Status", "Manufacturer Status", "Accountant Status"};
    ui->orderListTableWidget->setHorizontalHeaderLabels(headers);

    QList<QVariantList> orderList = DatabaseUtils::fetchOrderListDetails();
    if (orderList.isEmpty())
    {
        QMessageBox::information(this, "No Orders", "No orders found");
        return;
    }

    ui->orderListTableWidget->setRowCount(orderList.size());

    QStringList statusOptions = {"Pending", "Working", "Completed"};

    for (int row = 0; row < orderList.size(); ++row)
    {
        const QVariantList &order = orderList[row];

        // Sr No
        ui->orderListTableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));

        // User ID, Party ID, Job No (read-only)
        for (int col = 0; col <= 2; ++col)
        {
            QTableWidgetItem *item = new QTableWidgetItem(order[col].toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); // Make it read-only
            ui->orderListTableWidget->setItem(row, col + 1, item);
        }

        // Sr No (read-only)
        QTableWidgetItem *srItem = new QTableWidgetItem(QString::number(row + 1));
        srItem->setFlags(srItem->flags() & ~Qt::ItemIsEditable);
        ui->orderListTableWidget->setItem(row, 0, srItem);

        QString jobNo = order[2].toString(); // Column 2 is jobNo

        // Only make Designer Status (col=3) editable
        for (int col = 3; col <= 5; ++col)
        {
            QString currentStatus = order[col].toString(); // Must NOT be const

            if (col == 3)
            {
                QComboBox *combo = new QComboBox();
                combo->addItems(statusOptions);
                combo->setCurrentText(currentStatus);

                auto applyColor = [combo](const QString &status)
                {
                    QString color;
                    if (status == "Pending")
                        color = "#ffcccc";
                    else if (status == "Working")
                        color = "#fff5ba";
                    else if (status == "Completed")
                        color = "#ccffcc";

                    combo->setStyleSheet("QComboBox { background-color: " + color + "; }");
                };

                applyColor(currentStatus);

                connect(combo, &QComboBox::currentTextChanged, this,
                        [=, currentStatusCopy = currentStatus](const QString &newStatus) mutable
                        {
                            QStringList statusOrder = {"Pending", "Working", "Completed"};
                            int oldIndex = statusOrder.indexOf(currentStatusCopy);
                            int newIndex = statusOrder.indexOf(newStatus);

                            if (newIndex < oldIndex)
                            {
                                QMessageBox::StandardButton reply = QMessageBox::question(
                                    nullptr,
                                    "Admin Approval Needed",
                                    QString("Changing status from '%1' to '%2' is not allowed directly.\n"
                                            "Do you want to request this change from Admin?")
                                        .arg(currentStatusCopy, newStatus),
                                    QMessageBox::Yes | QMessageBox::No);

                                combo->blockSignals(true);
                                combo->setCurrentText(currentStatusCopy);
                                applyColor(currentStatusCopy);
                                combo->blockSignals(false);

                                if (reply == QMessageBox::Yes)
                                {
                                    if (reply == QMessageBox::Yes)
                                    {
                                        // Step 3: Save request to DB
                                        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "request_status");
                                        QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
                                        db.setDatabaseName(dbPath);

                                        if (!db.open())
                                        {
                                            qDebug() << "Failed to open DB for request: " << db.lastError().text();
                                            return;
                                        }

                                        QSqlQuery query(db);
                                        query.prepare(R"(
                                                        INSERT INTO StatusChangeRequests (jobNo, userId, fromStatus, toStatus, requestTime, role)
                                                        VALUES (:jobNo, :userId, :fromStatus, :toStatus, :requestTime, :role)
                                                    )");

                                        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

                                        query.bindValue(":jobNo", jobNo);
                                        query.bindValue(":userId", order[0].toString()); // Assuming userId is in column 0
                                        query.bindValue(":fromStatus", currentStatusCopy);
                                        query.bindValue(":toStatus", newStatus);
                                        query.bindValue(":requestTime", currentTime);
                                        query.bindValue(":role", "Designer");

                                        if (!query.exec())
                                        {
                                            qDebug() << "Failed to insert request: " << query.lastError().text();
                                        }
                                        else
                                        {
                                            QMessageBox::information(this, "Request Sent", "Your request has been recorded.");
                                        }

                                        db.close();
                                        QSqlDatabase::removeDatabase("request_status");
                                    }
                                }

                                return;
                            }

                            applyColor(newStatus);

                            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "update_status");
                            QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
                            db.setDatabaseName(dbPath);

                            if (!db.open())
                            {
                                qDebug() << "Failed to open DB: " << db.lastError().text();
                                return;
                            }

                            QSqlQuery updateQuery(db);
                            updateQuery.prepare("UPDATE \"Order-Status\" SET Designer = :status WHERE jobNo = :jobNo");
                            updateQuery.bindValue(":status", newStatus);
                            updateQuery.bindValue(":jobNo", jobNo);

                            if (!updateQuery.exec())
                            {
                                qDebug() << "Update failed: " << updateQuery.lastError().text();
                            }
                            else
                            {
                                currentStatusCopy = newStatus; // update local copy
                            }

                            db.close();
                            QSqlDatabase::removeDatabase("update_status");
                        });

                ui->orderListTableWidget->setCellWidget(row, col + 1, combo);
            }
            else
            {
                // Manufacturer & Accountant status are read-only
                QTableWidgetItem *item = new QTableWidgetItem(currentStatus);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui->orderListTableWidget->setItem(row, col + 1, item);
            }
        }
    }

    ui->orderListTableWidget->resizeColumnsToContents();
}
