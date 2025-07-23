#include "orderlist.h"
#include "ui_orderlist.h"

#include <QComboBox>
#include <databaseutils.h>
#include <QMessageBox>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QMenu>
#include <QFileDialog>
#include <QProgressDialog>

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

    ui->orderListTableWidget->setColumnCount(9); // Including Sr No
    QStringList headers = {"Sr No", "User ID", "Party ID", "Job No", "Designer Status", "Manufacturer Status", "Accountant Status", "Job Sheet","Print"};
    ui->orderListTableWidget->setHorizontalHeaderLabels(headers);
    setRoleAndUserRole(role);
    // qDebug()<<"---------"<<role;
    if (role == "designer") {
        show_order_list_with_role("Designer", 3);
    } else if (role == "manufacturer") {
        show_order_list_with_role("Manufacturer", 4);
    } else if (role == "accountant") {
        show_order_list_with_role("Accountant", 5);
    } else if (role == "Seller"){
        show_order_list_with_role("Seller", 1);
    }
}

void OrderList::setRoleAndUserInfo(const QString &role, const QString &userIdValue, const QString &userNameValue)
{
    userRole = role;
    userId = userIdValue;
    userName = userNameValue;
}

void OrderList::setRoleAndUserRole(const QString &role)
{
    userRole = role;
}

OrderList::~OrderList()
{
    delete ui;
}

void OrderList::populateCommonOrderRow(int row, const QVariantList &order)
{
    // Sr No (read-only)
    QTableWidgetItem *srItem = new QTableWidgetItem(QString::number(row + 1));
    srItem->setFlags(srItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 0, srItem);

    // User ID, Party ID, Job No (read-only)
    for (int col = 0; col <= 2; ++col)
    {
        QTableWidgetItem *item = new QTableWidgetItem(order[col].toString());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->orderListTableWidget->setItem(row, col + 1, item);
    }

    QString jobNo = order[2].toString(); // Job No assumed at col 2

    // Job Sheet Button
    QPushButton *jobSheetBtn = new QPushButton("Job Sheet");
    connect(jobSheetBtn, &QPushButton::clicked, this, [=]() {
        openJobSheet(jobNo);
    });
    ui->orderListTableWidget->setCellWidget(row, 7, jobSheetBtn);

    // Print Button
    QPushButton *printBtn = new QPushButton("Print");
    connect(printBtn, &QPushButton::clicked, this, [=]() {
        printJobSheet(jobNo);
    });
    ui->orderListTableWidget->setCellWidget(row, 8, printBtn);
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
    JobSheet *sheet = new JobSheet(this, jobNo, userRole); // <== pass userRole
    sheet->exec();
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

    QString templatePath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/sample_excel.xlsx");
    QString tempPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/temp_jobsheet_" + jobNo + ".xlsx");

    if (!QFile::copy(templatePath, tempPath)) {
        QMessageBox::critical(this, "Error", "Failed to create temporary Excel file.");
        return;
    }

    QString filePath = QDir::toNativeSeparators(tempPath);  // Use the copy for editing


    if (!QFile::exists(filePath)) {
        QMessageBox::critical(this, "Error", "Excel template file not found.");
        return;
    }

    // ✅ Show modal progress indicator
    QProgressDialog progress("Generating job sheet, please wait...", QString(), 0, 0, this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setCancelButton(nullptr);
    progress.setWindowTitle("Please Wait");
    progress.setMinimumDuration(0);
    progress.setRange(0, 0);
    progress.show();
    QApplication::processEvents();  // Makes sure it shows before Excel work starts



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

        QFile imageFile(imageRelPath);
        if (imageFile.exists()) {

            QAxObject *shapes = sheet->querySubObject("Shapes");

            // Optional: Reset working directory
            QDir::setCurrent(QCoreApplication::applicationDirPath());

            // qDebug() << "Full image path for Excel:" << nativeImagePath;

            // Get the top-left cell of the target range
            QAxObject *anchorCell = sheet->querySubObject("Range(const QString&)", "J2");
            double left = anchorCell->property("Left").toDouble();
            double top = anchorCell->property("Top").toDouble();

            // Get the total width and height of merged range J2:N9
            QAxObject *mergeRange = sheet->querySubObject("Range(const QString&)", "J2:N9");
            double width = mergeRange->property("Width").toDouble();
            double height = mergeRange->property("Height").toDouble();

            // Insert the image using accurate placement
            QAxObject *picture = shapes->querySubObject("AddPicture(const QString&, bool, bool, double, double, double, double)",
                                                        nativeImagePath,
                                                        false,  // LinkToFile
                                                        true,   // SaveWithDocument
                                                        left,
                                                        top,
                                                        width,
                                                        height);

            if (!picture || picture->isNull()) {
                QMessageBox::warning(this, "Image Insert Error", "Failed to insert image into Excel:\n" + nativeImagePath);
            }
        }

        // Save changes
        workbook->dynamicCall("Save()");

        // ✅ Print the document (silent print to default printer)
        // workbook->dynamicCall("PrintOut()");

        QString pdfPath = QCoreApplication::applicationDirPath() + "/jobSheet_" + jobNo + ".pdf";
        QString nativePdfPath = QDir::toNativeSeparators(pdfPath);

        qDebug() << "Attempting to export PDF to:" << nativePdfPath;

        if (!workbook || workbook->isNull()) {
            QMessageBox::critical(this, "Error", "Workbook is null. Cannot export.");
            return;
        }

        workbook->dynamicCall("ExportAsFixedFormat(int, const QString&)", 0, nativePdfPath);

        // Safer check: does the file exist after export?
        if (QFile::exists(nativePdfPath)) {
            QMessageBox::information(this, "PDF Exported", "Saved PDF to:\n" + nativePdfPath);
            QDesktopServices::openUrl(QUrl::fromLocalFile(nativePdfPath));
            qDebug() << "PDF export successful.";
        } else {
            QMessageBox::critical(this, "Error", "Failed to export PDF.\nThe file was not created:\n" + nativePdfPath);
            qDebug() << "PDF export failed. File does not exist.";
        }

        // Close Excel
        workbook->dynamicCall("Close()");
        excel->dynamicCall("Quit()");

        auto cleanupAxObject = [](QAxObject* obj) {
            if (obj) {
                obj->clear(); // Releases the COM object
                delete obj;
            }
        };
        // cleanupAxObject(picture);
        // cleanupAxObject(shapes);
        cleanupAxObject(sheet);
        cleanupAxObject(workbook);
        cleanupAxObject(workbooks);
        cleanupAxObject(excel);
    }
    // QMessageBox::information(this, "Success", "Excel updated and sent to printer.");

    db.close();
    QSqlDatabase::removeDatabase("set_jobsheet");

    if (QFile::exists(tempPath)) {
        QFile::remove(tempPath);
    }


}

void OrderList::show_order_list_with_role(const QString &role, int editableStatusCol)
{
    QList<QVariantList> orderList = DatabaseUtils::fetchOrderListDetails();
    if (orderList.isEmpty()) {
        QMessageBox::information(this, "No Orders", "No orders found");
        return;
    }

    ui->orderListTableWidget->setRowCount(orderList.size());
    // Hide irrelevant status columns based on role
    // qDebug()<<"---------"<<role;
    if (role == "Designer") {
        ui->orderListTableWidget->setColumnHidden(5, true); // Manufacturer Status
        ui->orderListTableWidget->setColumnHidden(6, true); // Accountant Status
    } else if (role == "Manufacturer") {
        ui->orderListTableWidget->setColumnHidden(4, true); // Designer Status
        ui->orderListTableWidget->setColumnHidden(6, true); // Accountant Status
    } else if (role == "Accountant") {
        ui->orderListTableWidget->setColumnHidden(4, true); // Designer Status
        ui->orderListTableWidget->setColumnHidden(5, true); // Manufacturer Status
    } else if (role == "Seller") {
        ui->orderListTableWidget->setColumnHidden(7, true); // Designer Status
        ui->orderListTableWidget->setColumnHidden(8, true); // Manufacturer Status
    }


    QStringList statusOptions = {"Pending", "Working", "Completed"};

    for (int row = 0; row < orderList.size(); ++row) {
        const QVariantList &order = orderList[row];

        if ((role == "Manufacturer" && order[3].toString() != "Completed") ||
            (role == "Accountant" && order[4].toString() != "Completed")) {
            ui->orderListTableWidget->setRowHidden(row, true);
            continue;
        }

        populateCommonOrderRow(row, order);

        QString jobNo = order[2].toString(); // Column 2 is jobNo

        for (int col = 3; col <= 5; ++col) {
            QString currentStatus = order[col].toString();

            if (col == editableStatusCol) {
                // Check if prerequisite role's status is "Completed"
                bool allowEdit = true;
                QString prerequisiteRoleStatus;

                if (role == "Manufacturer") {
                    prerequisiteRoleStatus = order[3].toString(); // Designer status at col=3
                    allowEdit = (prerequisiteRoleStatus == "Completed");
                } else if (role == "Accountant") {
                    prerequisiteRoleStatus = order[4].toString(); // Manufacturer status at col=4
                    allowEdit = (prerequisiteRoleStatus == "Completed");
                }

                QComboBox *combo = new QComboBox();
                combo->addItems(statusOptions);
                combo->setCurrentText(currentStatus);

                auto applyColor = [combo](const QString &status) {
                    QString color;
                    if (status == "Pending") color = "#ffcccc";
                    else if (status == "Working") color = "#fff5ba";
                    else if (status == "Completed") color = "#ccffcc";
                    combo->setStyleSheet("QComboBox { background-color: " + color + "; }");
                };
                applyColor(currentStatus);

                // Disable combo if not allowed
                if (!allowEdit) {
                    combo->setEnabled(false);
                    QString reason = (role == "Manufacturer")
                                         ? "Designer must complete their work first."
                                         : "Manufacturer must complete their work first.";
                    combo->setToolTip(reason);
                } else {
                    connect(combo, &QComboBox::currentTextChanged, this,
                            [=, currentStatusCopy = currentStatus](const QString &newStatus) mutable {
                                QStringList statusOrder = {"Pending", "Working", "Completed"};
                                int oldIndex = statusOrder.indexOf(currentStatusCopy);
                                int newIndex = statusOrder.indexOf(newStatus);

                                if (newIndex < oldIndex) {
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

                                    if (reply == QMessageBox::Yes) {
                                        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "request_status");
                                        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db"));

                                        if (!db.open()) {
                                            qDebug() << "Failed to open DB for request: " << db.lastError().text();
                                            return;
                                        }

                                        QSqlQuery query(db);
                                        query.prepare(R"(
                                INSERT INTO StatusChangeRequests (jobNo, userId, fromStatus, toStatus, requestTime, role)
                                VALUES (:jobNo, :userId, :fromStatus, :toStatus, :requestTime, :role)
                            )");

                                        query.bindValue(":jobNo", jobNo);
                                        query.bindValue(":userId", order[0].toString());
                                        query.bindValue(":fromStatus", currentStatusCopy);
                                        query.bindValue(":toStatus", newStatus);
                                        query.bindValue(":requestTime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
                                        query.bindValue(":role", role);

                                        if (!query.exec()) {
                                            qDebug() << "Failed to insert request: " << query.lastError().text();
                                        } else {
                                            QMessageBox::information(this, "Request Sent", "Your request has been recorded.");
                                        }

                                        db.close();
                                        QSqlDatabase::removeDatabase("request_status");
                                    }

                                    return;
                                }

                                applyColor(newStatus);

                                QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "update_status");
                                db.setDatabaseName(QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db"));

                                if (!db.open()) {
                                    qDebug() << "Failed to open DB: " << db.lastError().text();
                                    return;
                                }

                                QSqlQuery updateQuery(db);
                                updateQuery.prepare(QString("UPDATE \"Order-Status\" SET %1 = :status WHERE jobNo = :jobNo").arg(role));
                                updateQuery.bindValue(":status", newStatus);
                                updateQuery.bindValue(":jobNo", jobNo);

                                if (!updateQuery.exec()) {
                                    qDebug() << "Update failed: " << updateQuery.lastError().text();
                                } else {
                                    currentStatusCopy = newStatus;
                                }

                                db.close();
                                QSqlDatabase::removeDatabase("update_status");
                            });
                }

                ui->orderListTableWidget->setCellWidget(row, col + 1, combo);
            }
            else {
                QTableWidgetItem *item = new QTableWidgetItem(currentStatus);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui->orderListTableWidget->setItem(row, col + 1, item);
            }
        }
    }

    ui->orderListTableWidget->resizeColumnsToContents();
}
