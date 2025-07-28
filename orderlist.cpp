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
#include <QInputDialog>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "jobsheet.h"

#include "header/xlsxdocument.h"

using namespace QXlsx;
// RPD  Casting Bagging
OrderList::OrderList(QWidget *parent, const QString &role)
    : QDialog(parent), ui(new Ui::OrderList)
{

    ui->setupUi(this);

    ui->orderListTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->orderListTableWidget, &QTableWidget::customContextMenuRequested,
            this, &OrderList::onTableRightClick);

    QStringList headers = {
        "Sr No", "User ID", "Party ID", "Job No",
        "Manager Status", "Designer Status", "Manufacturer Status", "Accountant Status",
        "Order Date", "Delivery Date",
        "Job Sheet", "Print", "Image",
        "Order Approval", "Design Approval", "Quality Approval",
        "Order Note", "Design Note", "Quality Note"
    };
    ui->orderListTableWidget->setColumnCount(headers.size());
    ui->orderListTableWidget->setHorizontalHeaderLabels(headers);
    ui->orderListTableWidget->setSortingEnabled(true);  // ✅ Add this line
    ui->orderListTableWidget->verticalHeader()->setVisible(false);
    setRoleAndUserRole(role);
    // qDebug()<<"---------"<<role;
    if (role == "designer") {
        show_order_list_with_role("designer", 4);
    } else if (role == "manufacturer") {
        show_order_list_with_role("manufacturer", 5);
    } else if (role == "accountant") {
        show_order_list_with_role("accountant", 6);
    } else if (role == "Seller"){
        // qDebug()<<"---------"<<role;
        show_order_list_with_role("seller", 1);
    } else if (role == "manager") {
        show_order_list_with_role("manager", 3); // editable column index for manager
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
    if (order.size() < 10) {
        qDebug() << "❌ Error: order list size too small:" << order.size();
        return;
    }

    // Sr No
    QTableWidgetItem *srItem = new QTableWidgetItem(QString::number(row + 1));
    srItem->setFlags(srItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 0, srItem);

    // User ID, Party ID, Job No
    for (int col = 0; col <= 2; ++col)
    {
        QTableWidgetItem *item = new QTableWidgetItem(order[col].toString());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->orderListTableWidget->setItem(row, col + 1, item);
    }

    QString jobNo = order[2].toString();

    // Order Date
    QTableWidgetItem *orderDateItem = new QTableWidgetItem(order[7].toString());
    orderDateItem->setFlags(orderDateItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 8, orderDateItem);

    // Delivery Date
    QTableWidgetItem *deliveryDateItem = new QTableWidgetItem(order[8].toString());
    deliveryDateItem->setFlags(deliveryDateItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 9, deliveryDateItem);

    // Job Sheet Button
    QPushButton *jobSheetBtn = new QPushButton("Job Sheet");
    connect(jobSheetBtn, &QPushButton::clicked, this, [=]() {
        openJobSheet(jobNo);
    });
    ui->orderListTableWidget->setCellWidget(row, 10, jobSheetBtn);

    // Print Button
    QPushButton *printBtn = new QPushButton("Print");
    connect(printBtn, &QPushButton::clicked, this, [=]() {
        printJobSheet(jobNo);
    });
    ui->orderListTableWidget->setCellWidget(row, 11, printBtn);

    // Show Image Button
    QPushButton *showImgBtn = new QPushButton("Show Img");
    connect(showImgBtn, &QPushButton::clicked, this, [=]() {
        QString imagePath = QCoreApplication::applicationDirPath() + "/" + order[9].toString();
        QPixmap pixmap(imagePath);
        if (pixmap.isNull()) {
            QMessageBox::warning(this, "Image Error", "⚠️ Failed to load image:\n" + imagePath);
            return;
        }

        QLabel *label = new QLabel;
        label->setPixmap(pixmap.scaled(600, 600, Qt::KeepAspectRatio));
        label->setWindowTitle("Image Preview: " + jobNo);
        label->setAttribute(Qt::WA_DeleteOnClose);
        label->show();
    });
    ui->orderListTableWidget->setCellWidget(row, 12, showImgBtn); // Place in new column 12

    int approvalColStart = 13;
    ui->orderListTableWidget->setItem(row, approvalColStart,     new QTableWidgetItem(order[10].toString())); // Order_Approve
    ui->orderListTableWidget->setItem(row, approvalColStart + 1, new QTableWidgetItem(order[11].toString())); // Design_Approve
    ui->orderListTableWidget->setItem(row, approvalColStart + 2, new QTableWidgetItem(order[12].toString())); // Quality_Approve
    ui->orderListTableWidget->setItem(row, approvalColStart + 3, new QTableWidgetItem(order[13].toString())); // Order_Note
    ui->orderListTableWidget->setItem(row, approvalColStart + 4, new QTableWidgetItem(order[14].toString())); // Design_Note
    ui->orderListTableWidget->setItem(row, approvalColStart + 5, new QTableWidgetItem(order[15].toString())); // Quality_Note


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

// Cleaned-up and modularized version of printJobSheet
void OrderList::printJobSheet(const QString &jobNo) {
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // --- Open Order DB ---
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

    if (!query.exec() || !query.next()) {
        qDebug() << "Query failed or no record found:" << query.lastError().text();
        return;
    }

    // --- Prepare Excel Template ---
    QString templatePath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/sample_excel.xlsx");
    QString tempPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/temp_jobsheet_" + jobNo + ".xlsx");
    if (!QFile::copy(templatePath, tempPath)) {
        QMessageBox::critical(this, "Error", "Failed to create temporary Excel file.");
        return;
    }

    QProgressDialog progress("Generating job sheet, please wait...", QString(), 0, 0, this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setCancelButton(nullptr);
    progress.setWindowTitle("Please Wait");
    progress.setMinimumDuration(0);
    progress.setRange(0, 0);
    progress.show();
    QApplication::processEvents();

    // --- Launch Excel ---
    QAxObject *excel = new QAxObject("Excel.Application", this);
    if (!excel || excel->isNull()) {
        QMessageBox::critical(this, "Error", "Excel is not available.");
        return;
    }
    excel->setProperty("DisplayAlerts", false);
    excel->setProperty("ScreenUpdating", false);
    excel->setProperty("EnableEvents", false);
    excel->setProperty("Visible", false);

    QAxObject *workbooks = excel->querySubObject("Workbooks");
    QAxObject *workbook = workbooks->querySubObject("Open(const QString&)", tempPath);
    QAxObject *sheet = workbook->querySubObject("Worksheets(int)", 1);
    //Auto Fit
    QAxObject *range = sheet->querySubObject("Range(const QString&)", "J11:M26");
    QAxObject *columns = range->querySubObject("EntireColumn");
    columns->dynamicCall("AutoFit()");


    // --- Fill Excel Template ---
    auto setCell = [&](const QString &cell, const QVariant &value, const QString &format = QString()) {
        QAxObject *r = sheet->querySubObject("Range(const QString&)", cell);
        r->setProperty("Value", value);
        if (!format.isEmpty()) r->setProperty("NumberFormat", format);
    };

    setCell("B3", query.value("partyId"));
    setCell("H5", query.value("jobNo"));
    setCell("H4", query.value("orderNo"));
    setCell("H3", query.value("clientId"));
    setCell("H2", QDate::fromString(query.value("orderDate").toString(), "yyyy-MM-dd").toString("MM/dd/yyyy"), "mm/dd/yyyy");
    setCell("A6", QDate::fromString(query.value("deliveryDate").toString(), "yyyy-MM-dd").toString("MM/dd/yyyy"), "mm/dd/yyyy");

    setCell("B4", query.value("productPis"));
    setCell("F4", query.value("designNo1"));
    setCell("B6", query.value("metalPurity"));
    setCell("C6", query.value("metalColor"));
    setCell("D6", query.value("sizeNo"));
    setCell("E6", query.value("sizeMM"));
    setCell("F6", query.value("length"));
    setCell("G6", query.value("width"));
    setCell("H6", query.value("height"));

    // --- Insert Image ---
    QString imagePath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/" + query.value("image1path").toString());
    if (QFile::exists(imagePath)) {
        QAxObject *shapes = sheet->querySubObject("Shapes");
        QAxObject *anchorCell = sheet->querySubObject("Range(const QString&)", "J2");
        double left = anchorCell->property("Left").toDouble();
        double top = anchorCell->property("Top").toDouble();
        QAxObject *range = sheet->querySubObject("Range(const QString&)", "J2:N9");
        double width = range->property("Width").toDouble();
        double height = range->property("Height").toDouble();

        shapes->querySubObject("AddPicture(const QString&, bool, bool, double, double, double, double)",
                               imagePath, false, true, left, top, width, height);
    }

    // --- Load and Insert Stones Table ---
    QString designNo = query.value("designNo1").toString();
    QSqlDatabase imageDb = QSqlDatabase::addDatabase("QSQLITE", "image_conn_excel");
    imageDb.setDatabaseName("database/mega_mine_image.db");

    if (imageDb.open()) {
        QSqlQuery imageQuery(imageDb);
        imageQuery.prepare("SELECT diamond, stone FROM image_data WHERE design_no = :designNo");
        imageQuery.bindValue(":designNo", designNo);

        if (imageQuery.exec() && imageQuery.next()) {
            auto insertStones = [&](const QString &json, const QString &label, int &row) {
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
                if (err.error != QJsonParseError::NoError || !doc.isArray()) {
                    qDebug() << "❌ JSON parse error for" << label << ":" << err.errorString();
                    return;
                }

                QJsonArray arr = doc.array();
                for (const QJsonValue &v : arr) {
                    if (!v.isObject()) continue;

                    QJsonObject o = v.toObject();
                    QString type = o.value("type").toString();
                    QString quantity = o.value("quantity").toString();
                    QString size = o.value("sizeMM").toString();

                    setCell(QString("J%1").arg(row), label);    // Column J: "Diamond"/"Stone"
                    setCell(QString("L%1").arg(row), type);     // Column K: Type (e.g. Round)
                    setCell(QString("N%1").arg(row), quantity); // Column L: Quantity
                    setCell(QString("P%1").arg(row), size);     // Column M: Size

                    if (++row > 26) break; // Avoid infinite overflow
                }
            };

            // 🔰 Set header row (optional)
            setCell("J11", "Type");
            setCell("L11", "Name");
            setCell("N11", "Qty");
            setCell("P11", "Size");

            // ⬇ Insert data
            int rowNum = 12;
            insertStones(imageQuery.value("diamond").toString(), "Diamond", rowNum);
            insertStones(imageQuery.value("stone").toString(), "Stone", rowNum);

            // // Add border to the table after all stones inserted
            int firstRow = 11;
            int lastRow = rowNum - 1;

            if (lastRow >= firstRow) {
                QString rangeStr = QString("J%1:Q%2").arg(firstRow).arg(lastRow);
                QAxObject* range = sheet->querySubObject("Range(const QString&)", rangeStr);
                if (!range) {
                    qDebug() << "Failed to get Range object";
                    return;
                }

                QAxObject* borders = range->querySubObject("Borders");
                if (!borders) {
                    qDebug() << "Failed to get Borders object";
                    return;
                }

                QList<int> borderIndices = {7, 8, 9, 10, 11, 12}; // xlEdgeLeft..InsideHorizontal

                for (int index : borderIndices) {
                    QAxObject* border = borders->querySubObject("Item(int)", index);
                    if (border) {
                        border->setProperty("LineStyle", 1);  // xlContinuous
                        border->setProperty("Weight", -4138); // xlThin
                        border->dynamicCall("Release");
                    } else {
                        qDebug() << "❌ Failed to get border for index" << index;
                    }
                }
            }

        }

        imageDb.close();
        QSqlDatabase::removeDatabase("image_conn_excel");
    } else {
        qDebug() << "❌ Could not open image DB for stones.";
    }


    workbook->dynamicCall("Save()");
    QString pdfPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/jobSheet_" + jobNo + ".pdf");
    workbook->dynamicCall("ExportAsFixedFormat(int, const QString&)", 0, pdfPath);

    if (QFile::exists(pdfPath)) {
        QMessageBox::information(this, "PDF Exported", "Saved PDF to:\n" + pdfPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
    } else {
        QMessageBox::critical(this, "Error", "Failed to export PDF:" + pdfPath);
    }

    workbook->dynamicCall("Close()");
    excel->dynamicCall("Quit()");

    delete sheet;
    delete workbook;
    delete workbooks;
    delete excel;

    db.close();
    QSqlDatabase::removeDatabase("set_jobsheet");
    if (QFile::exists(tempPath)) QFile::remove(tempPath);
}



void OrderList::setupStatusCombo(int row, int col, const QString &role, const QString &currentStatus,
                                 const QString &jobNo, const QVariantList &order, const int &editableStatusCol)
{
    QStringList statusOptions = getStatusOptions(role);
    QComboBox *combo = new QComboBox();

    QString managerStatus      = order[3].toString();
    QString designerStatus     = order[4].toString();
    QString manufacturerStatus = order[5].toString();

    QStringList allowedStates;
    QStringList possibleStates = getStatusOptions(role);  // Full ordered list

    int currentIndex = possibleStates.indexOf(currentStatus);

    // ✅ Always show all previous states
    for (int i = 0; i <= currentIndex; ++i) {
        allowedStates << possibleStates[i];
    }

    // ✅ Then add only the valid next forward state
    if (role == "manager") {
        if (currentStatus == "Pending") {
            allowedStates << "Order Checked";
        } else if (currentStatus == "Order Checked" && designerStatus == "Completed") {
            allowedStates << "Design Checked";
        } else if (currentStatus == "Design Checked") {
            allowedStates << "RPD";
        } else if (currentStatus == "RPD") {
            allowedStates << "Casting";
        } else if (currentStatus == "Casting") {
            allowedStates << "Bagging";
        } else if (currentStatus == "Bagging" && manufacturerStatus == "Completed") {
            allowedStates << "QC Done";
        }
    } else {
        allowedStates = possibleStates;  // For other roles
    }

    allowedStates.removeDuplicates();  // Just in case
    combo->addItems(allowedStates);
    combo->setCurrentText(currentStatus);

    auto applyColor = [combo](const QString &status) {
        QString color;
        if      (status == "Pending")        color = "#ffcccc";   // Light red
        else if (status == "Working")        color = "#fff5ba";   // Light yellow
        else if (status == "Completed")      color = "#ccffcc";   // Light green
        else if (status == "Order Checked")  color = "#ffd8a8";   // Light orange
        else if (status == "Design Checked") color = "#fff5ba";   // Light yellow
        else if (status == "RPD")            color = "#cce5ff";   // Light blue
        else if (status == "Casting")        color = "#d1c4e9";   // Light purple
        else if (status == "Bagging")        color = "#e6ee9c";   // Light lime
        else if (status == "QC Done")        color = "#ccffcc";   // Light green
        combo->setStyleSheet("QComboBox { background-color: " + color + "; }");
    };

    applyColor(currentStatus);

    // --- Permission Check ---
    bool forwardAllowed = false;

    if (role == "manager") {
        if (managerStatus == "Pending") forwardAllowed = true;
        else if (managerStatus == "Order Checked" && designerStatus == "Completed") forwardAllowed = true;
        else if (managerStatus == "Design Checked") forwardAllowed = true;
        else if (managerStatus == "RPD") forwardAllowed = true;
        else if (managerStatus == "Casting") forwardAllowed = true;
        else if (managerStatus == "Bagging" && manufacturerStatus == "Completed") forwardAllowed = true;
    }

    // ✅ Manager can always edit combo box (to select prev state for request)
    bool allowEdit = (role == "manager") ||
                     (role == "designer" && managerStatus == "Order Checked") ||
                     (role == "manufacturer" && managerStatus == "Bagging") ||
                     (role == "accountant" && managerStatus == "QC Done");


    if (!allowEdit) {
        combo->setEnabled(false);
        QString reason;
        if (role == "designer")      reason = "manager has not yet Order Checked this design.";
        else if (role == "manufacturer") reason = "designer must complete their work first.";
        else if (role == "accountant")   reason = "manufacturer must complete the job first.";
        else if (role == "manager")      reason = "This is not manager’s current stage.";
        combo->setToolTip(reason);
    } else {
        connect(combo, &QComboBox::currentTextChanged, this,
                [=, currentStatusCopy = currentStatus](const QString &newStatus) mutable {

                    QStringList statusOrder = getStatusOptions(role);
                    int oldIndex = statusOrder.indexOf(currentStatusCopy);
                    int newIndex = statusOrder.indexOf(newStatus);

                    // Prevent backward transition
                    if (newIndex < oldIndex) {
                        QMessageBox::StandardButton reply = QMessageBox::question(
                            nullptr,
                            "Admin Approval Needed",
                            QString("Changing status from '%1' to '%2' is not allowed directly.\n"
                                    "Do you want to request this change from Admin?")
                                .arg(currentStatusCopy, newStatus),
                            QMessageBox::Yes | QMessageBox::No
                            );

                        combo->blockSignals(true);
                        combo->setCurrentText(currentStatusCopy);
                        applyColor(currentStatusCopy);
                        combo->blockSignals(false);

                        if (reply == QMessageBox::Yes) {
                            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "request_status");
                            db.setDatabaseName(QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db"));

                            if (db.open()) {
                                // 🟡 Ask for optional note
                                QString note = QInputDialog::getText(
                                    this,
                                    "Request Note",
                                    "Optionally enter a note for this request:",
                                    QLineEdit::Normal,
                                    ""
                                    );

                                QSqlQuery query(db);
                                query.prepare(R"(
                                                INSERT INTO StatusChangeRequests
                                                (jobNo, userId, fromStatus, toStatus, requestTime, role, note)
                                                VALUES
                                                (:jobNo, :userId, :fromStatus, :toStatus, :requestTime, :role, :note)
                                            )");

                                query.bindValue(":jobNo", jobNo);
                                query.bindValue(":userId", userId);
                                query.bindValue(":fromStatus", currentStatusCopy);
                                query.bindValue(":toStatus", newStatus);
                                query.bindValue(":requestTime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
                                query.bindValue(":role", role);
                                query.bindValue(":note", note);

                                if (query.exec()) {
                                    QMessageBox::information(this, "Request Sent", "Your request has been recorded.");
                                } else {
                                    qDebug() << "Failed to insert request: " << query.lastError().text();
                                }

                                db.close();
                                QSqlDatabase::removeDatabase("request_status");
                            } else {
                                qDebug() << "Failed to open DB for request: " << db.lastError().text();
                            }
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

                    bool allowStatusChange = true;

                    if (role == "manager") {
                        if (newStatus == "Order Checked") {
                            QMessageBox msgBox(this);
                            msgBox.setWindowTitle("Approve Order?");
                            msgBox.setText("Do you approve the order check?");
                            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

                            // Remove close button
                            msgBox.setWindowFlags(msgBox.windowFlags() & ~Qt::WindowCloseButtonHint);

                            // Optional: Make it modal and disable ESC key
                            msgBox.setWindowModality(Qt::ApplicationModal);
                            msgBox.setEscapeButton(nullptr);  // Prevent ESC key from closing

                            int reply = msgBox.exec();
                            int approved = (reply == QMessageBox::Yes) ? 1 : 0;
                            QString note = "";
                            if(approved == 0){
                                note = QInputDialog::getText(this, "Rejection Note", "Please enter reason:");
                                allowStatusChange = false;
                            }
                            QSqlQuery query(db);
                            query.prepare(R"(UPDATE "Order-Status"
                                             SET Order_Approve = :approved,
                                                 Order_Note = CASE WHEN :approved = 1 THEN '' ELSE :note END
                                             WHERE jobNo = :jobNo)");
                            query.bindValue(":approved", approved);
                            query.bindValue(":note", note);
                            query.bindValue(":jobNo", jobNo);
                            query.exec();
                        } else if (newStatus == "Design Checked") {
                            QMessageBox msgBox(this);
                            msgBox.setWindowTitle("Approve Design?");
                            msgBox.setText("Do you approve the design check?");
                            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

                            // Remove close button
                            msgBox.setWindowFlags(msgBox.windowFlags() & ~Qt::WindowCloseButtonHint);

                            // Optional: Make it modal and disable ESC key
                            msgBox.setWindowModality(Qt::ApplicationModal);
                            msgBox.setEscapeButton(nullptr);  // Prevent ESC key from closing

                            int reply = msgBox.exec();
                            int approved = (reply == QMessageBox::Yes) ? 1 : 0;
                            QString note = "";
                            if(approved == 0){
                                note = QInputDialog::getText(this, "Rejection Note", "Please enter reason:");
                                allowStatusChange = false;
                            }
                            QSqlQuery query(db);
                            query.prepare(R"(UPDATE "Order-Status"
                                             SET Design_Approve = :approved,
                                                 Design_Note = CASE WHEN :approved = 1 THEN '' ELSE :note END,
                                                 Designer = CASE WHEN :approved = 0 THEN 'Working' ELSE Designer END
                                             WHERE jobNo = :jobNo)");
                            query.bindValue(":approved", approved);
                            query.bindValue(":note", note);
                            query.bindValue(":jobNo", jobNo);
                            query.exec();
                        } else if (newStatus == "QC Done") {
                            QMessageBox msgBox(this);
                            msgBox.setWindowTitle("Approve Quality?");
                            msgBox.setText("Do you approve the quality check?");
                            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

                            // Remove close button
                            msgBox.setWindowFlags(msgBox.windowFlags() & ~Qt::WindowCloseButtonHint);

                            // Optional: Make it modal and disable ESC key
                            msgBox.setWindowModality(Qt::ApplicationModal);
                            msgBox.setEscapeButton(nullptr);  // Prevent ESC key from closing

                            int reply = msgBox.exec();
                            int approved = (reply == QMessageBox::Yes) ? 1 : 0;
                            QString note = "";
                            if(approved == 0){
                                note = QInputDialog::getText(this, "Rejection Note", "Please enter reason:");
                                allowStatusChange = false;
                            }
                            QSqlQuery query(db);
                            query.prepare(R"(UPDATE "Order-Status"
                                             SET Quality_Approve = :approved,
                                                 Quality_Note = CASE WHEN :approved = 1 THEN '' ELSE :note END,
                                                 Manufacturer = CASE WHEN :approved = 0 THEN 'Working' ELSE Manufacturer END
                                             WHERE jobNo = :jobNo)");
                            query.bindValue(":approved", approved);
                            query.bindValue(":note", note);
                            query.bindValue(":jobNo", jobNo);
                            query.exec();
                        }
                    }

                    if (allowStatusChange) {
                        QSqlQuery updateQuery(db);
                        updateQuery.prepare(QString("UPDATE \"Order-Status\" SET %1 = :status WHERE jobNo = :jobNo").arg(role));
                        updateQuery.bindValue(":status", newStatus);
                        updateQuery.bindValue(":jobNo", jobNo);

                        if (updateQuery.exec()) {
                            currentStatusCopy = newStatus;
                            // qDebug() << "Reloading UI for role:" << userRole << "col:" << editableStatusCol;
                            show_order_list_with_role(userRole, editableStatusCol);
                        } else {
                            qDebug() << "Update failed: " << updateQuery.lastError().text();
                        }
                    } else {show_order_list_with_role(userRole, editableStatusCol);}
                    db.close();
                    QSqlDatabase::removeDatabase("update_status");
                });
    }

    ui->orderListTableWidget->setCellWidget(row, col, combo);
}

bool OrderList::shouldShowRow(const QString &role, const QVariantList &order)
{
    QString managerStatus      = order[3].toString();
    // QString designerStatus     = order[4].toString();
    // QString manufacturerStatus = order[5].toString();

    if (role == "designer" && managerStatus == "Order Checked")
        return true;

    if (role == "manufacturer" && managerStatus == "Bagging")
        return true;

    if (role == "accountant" && managerStatus == "QC Done")
        return true;

    if (role == "manager")
        return true;

    if (role == "seller")
        return true;

    return false;
}

QStringList OrderList::getStatusOptions(const QString &role)
{
    if (role == "manager") {
        return { "Pending", "Order Checked", "Design Checked", "RPD", "Casting", "Bagging", "QC Done" };
    } else {
        return { "Pending", "Working", "Completed" };
    }
}

void OrderList::hideIrrelevantColumns(const QString &role)
{
    QMap<QString, QList<int>> roleColumnMap = {
        { "designer",      {4, 6, 7, 13, 15} },
        { "manufacturer",  { 4, 5, 7, 13, 14} },
        { "accountant",    {4, 5, 6, 12} },
        { "seller",        {10, 11, 12, 13, 14, 15, 17, 18} },
        { "manager",       {12} }
    };

    for (int col = 0; col < ui->orderListTableWidget->columnCount(); ++col) {
        ui->orderListTableWidget->setColumnHidden(col, false); // Show all first
    }

    if (roleColumnMap.contains(role)) {
        for (int col : roleColumnMap[role]) {
            ui->orderListTableWidget->setColumnHidden(col, true);
        }
    }
}

void OrderList::show_order_list_with_role(const QString &role, int editableStatusCol)
{
    QList<QVariantList> orderList = DatabaseUtils::fetchOrderListDetails();
    if (orderList.isEmpty()) {
        QMessageBox::information(this, "No Orders", "No orders found");
        return;
    }
    // qDebug()<<"---------"<<role;
    ui->orderListTableWidget->setRowCount(orderList.size());

    hideIrrelevantColumns(role);
    QStringList statusOptions = getStatusOptions(role);

    for (int row = 0; row < orderList.size(); ++row) {
        const QVariantList &order = orderList[row];

        if (!shouldShowRow(role, order)) {
            ui->orderListTableWidget->setRowHidden(row, true);
            continue;
        }

        populateCommonOrderRow(row, order);
        QString jobNo = order[2].toString();

        for (int col = 3; col <= 6; ++col) {

            QString currentStatus = order[col].toString();
            if (col == editableStatusCol && editableStatusCol != 1) {
                setupStatusCombo(row, col + 1, role, currentStatus, jobNo, order, editableStatusCol);
            } else {
                QTableWidgetItem *item = new QTableWidgetItem(currentStatus);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui->orderListTableWidget->setItem(row, col + 1, item);
            }
        }
    }

    ui->orderListTableWidget->resizeColumnsToContents();
}
