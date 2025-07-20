#include "orderlist.h"
#include "ui_orderlist.h"

#include <QComboBox>
#include <databaseutils.h>
#include <QMessageBox>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QMenu>

#include <QAxObject>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>


#include "jobsheet.h"

#include "header/xlsxdocument.h"

using namespace QXlsx;

OrderList::OrderList(QWidget *parent, const QString &role)
    : QDialog(parent), ui(new Ui::OrderList)
{
    ui->setupUi(this);

    ui->orderListTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->orderListTableWidget, &QTableWidget::customContextMenuRequested,
            this, &OrderList::onTableRightClick);


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

        int row = index.row();
        int jobNoColumn = 3;  // Replace with actual column index if different

        QTableWidgetItem *jobNoItem = ui->orderListTableWidget->item(row, jobNoColumn);
        QString jobNo;

        if (jobNoItem)
            jobNo = jobNoItem->text();
        else
            jobNo = ""; // Fallback if cell is empty

        if (jobNo.isEmpty()) {
            QMessageBox::warning(this, "Missing Data", "Job No is empty.");
            return;
        }

        openJobSheet(jobNo);

    }
}

void OrderList::openJobSheet(const QString &jobNo){
    JobSheet *sheet = new JobSheet(this, jobNo);
    sheet->exec(); // Just open the dialog
}

void OrderList::printJobSheet(const QString &jobNo){
    // Set working directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // Open database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "set_jobsheet");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);
    query.prepare(R"(
                SELECT partyId, jobNo, orderNo, clientId, orderDate, deliveryDate,
                       productPis, designNo1, metalPurity, metalColor, sizeNo, sizeMM,
                       length, width, height, image1path
                FROM "OrderBook-Detail"
                WHERE jobNo = :jobNo
    )");

    query.bindValue(":jobNo", jobNo);

    if (!query.exec()) {
        qDebug() << "Query failed:" << query.lastError().text();
        return;
    }

    // QString filePath = "sample_excel.xlsx";
    QString filePath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/sample_excel.xlsx");


    if (!QFile::exists(filePath)) {
        QMessageBox::critical(this, "Error", "Excel template file not found.");
        return;
    }


    // QAxObject *excel = new QAxObject("Excel.Application", this);
    QAxObject *excel = new QAxObject("Excel.Application", this);
    if (excel->isNull()) {
        QMessageBox::critical(this, "Error", "Excel is not available on this system.");
        return;
    }

    // 🚀 Optimization: Disable slow Excel features
    excel->setProperty("DisplayAlerts", false);     // Prevents popups like "Save changes?"
    excel->setProperty("ScreenUpdating", false);    // Avoids UI redraw in Excel
    excel->setProperty("EnableEvents", false);      // Disables background event triggers

    excel->setProperty("Visible", false);

    QAxObject *workbooks = excel->querySubObject("Workbooks");
    QAxObject *workbook = workbooks->querySubObject("Open(const QString&)", filePath);
    QAxObject *sheet = workbook->querySubObject("Worksheets(int)", 1); // Sheet1


    if(query.next()){
        sheet->querySubObject("Range(const QString&)", "B3")->setProperty("Value", query.value("partyId").toString());
        sheet->querySubObject("Range(const QString&)", "H5")->setProperty("Value", query.value("jobNo").toString());
        sheet->querySubObject("Range(const QString&)", "H4")->setProperty("Value", query.value("orderNo").toString());
        sheet->querySubObject("Range(const QString&)", "H3")->setProperty("Value", query.value("clientId").toString());

        // Read and parse order date
        QString rawOrderDate = query.value("orderDate").toString();
        QDate orderDate = QDate::fromString(rawOrderDate, "yyyy-MM-dd");  // Correct format

        if (orderDate.isValid()) {
            QAxObject *orderCell = sheet->querySubObject("Range(const QString&)", "H2");
            orderCell->setProperty("Value", orderDate.toString("MM/dd/yyyy"));  // Excel-compatible
            orderCell->setProperty("NumberFormat", "mm/dd/yyyy");               // Force Excel to treat it as date
        } else {
            qWarning() << "Invalid order date:" << rawOrderDate;
        }

        // Read and parse delivery date
        QString rawDeliveryDate = query.value("deliveryDate").toString();
        QDate deliveryDate = QDate::fromString(rawDeliveryDate, "yyyy-MM-dd");  // Correct format

        if (deliveryDate.isValid()) {
            QAxObject *deliveryCell = sheet->querySubObject("Range(const QString&)", "A6");
            deliveryCell->setProperty("Value", deliveryDate.toString("MM/dd/yyyy"));
            deliveryCell->setProperty("NumberFormat", "mm/dd/yyyy");
        } else {
            qWarning() << "Invalid delivery date:" << rawDeliveryDate;
        }



        sheet->querySubObject("Range(const QString&)", "B4")->setProperty("Value", QString::number(query.value("productPis").toInt()));
        sheet->querySubObject("Range(const QString&)", "F4")->setProperty("Value", query.value("designNo1").toString());
        sheet->querySubObject("Range(const QString&)", "B6")->setProperty("Value", query.value("metalPurity").toString());
        sheet->querySubObject("Range(const QString&)", "C6")->setProperty("Value", query.value("metalColor").toString());
        sheet->querySubObject("Range(const QString&)", "D6")->setProperty("Value", QString::number(query.value("sizeNo").toDouble()));
        sheet->querySubObject("Range(const QString&)", "E6")->setProperty("Value", QString::number(query.value("sizeMM").toDouble()));
        sheet->querySubObject("Range(const QString&)", "F6")->setProperty("Value", QString::number(query.value("length").toDouble()));
        sheet->querySubObject("Range(const QString&)", "G6")->setProperty("Value", QString::number(query.value("width").toDouble()));
        sheet->querySubObject("Range(const QString&)", "H6")->setProperty("Value", QString::number(query.value("height").toDouble()));

        // --- IMAGE HANDLING ---
        QString imageRelPath = query.value("image1path").toString();
        QString imageFullPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/" + imageRelPath);
        QString nativeImagePath = QDir::toNativeSeparators(imageFullPath); // ✅ REQUIRED for Excel/Windows

        QFile imageFile(nativeImagePath);
        if (!imageFile.exists()) {
            QMessageBox::warning(this, "Image Not Found", "Image not found at:\n" + nativeImagePath);
        } else {
            QAxObject *shapes = sheet->querySubObject("Shapes");

            // Optional: Reset working directory
            QDir::setCurrent(QCoreApplication::applicationDirPath());

            // qDebug() << "Full image path for Excel:" << nativeImagePath;

            QAxObject *picture = shapes->querySubObject("AddPicture(const QString&, bool, bool, double, double, double, double)",
                                                        nativeImagePath,
                                                        false,  // LinkToFile
                                                        true,   // SaveWithDocument
                                                        570.0,  // Left (column J)
                                                        20.0,   // Top (row 2)
                                                        172.0,  // Width (J to N)
                                                        130.0); // Height (rows 2–9)

            if (!picture || picture->isNull()) {
                QMessageBox::warning(this, "Image Insert Error", "Failed to insert image into Excel:\n" + nativeImagePath);
            }
        }



        // Save changes
        workbook->dynamicCall("Save()");

        // ✅ Print the document (silent print to default printer)
        workbook->dynamicCall("PrintOut()");

        // Close Excel
        workbook->dynamicCall("Close()");
        excel->dynamicCall("Quit()");

        delete excel;

    }
    QMessageBox::information(this, "Success", "Excel updated and sent to printer.");

    db.close();
    QSqlDatabase::removeDatabase("set_jobsheet");
}

void OrderList::show_order_list_designer()
{
    ui->orderListTableWidget->setColumnCount(9); // Including Sr No
    QStringList headers = {"Sr No", "User ID", "Party ID", "Job No", "Designer Status", "Manufacturer Status", "Accountant Status", "Job Sheet","Print"};
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

        // Job Sheet Button
        QPushButton *jobSheetBtn = new QPushButton("Job Sheet");
        connect(jobSheetBtn, &QPushButton::clicked, this, [=]() {
            QString jobNoForBtn = order[2].toString(); // Assuming column 2 is Job No

            openJobSheet(jobNoForBtn);
        });
        ui->orderListTableWidget->setCellWidget(row, 7, jobSheetBtn); // Column index 7

        // Print Button
        QPushButton *printBtn = new QPushButton("Print");
        connect(printBtn, &QPushButton::clicked, this, [=]() {
            QString jobNoForBtn = order[2].toString();
            // QMessageBox::information(this, "Print", "Print clicked for Job No: " + jobNoForBtn);
            printJobSheet(jobNoForBtn);
            // TODO: Implement actual Print logic
        });
        ui->orderListTableWidget->setCellWidget(row, 8, printBtn); // Column index 8

    }

    ui->orderListTableWidget->resizeColumnsToContents();
}
