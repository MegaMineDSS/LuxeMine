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
#include <winerror.h>
#include <qt_windows.h>
#include <QProcess>
#include <QThread>

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
        show_order_list_with_role("designer", 5);
    } else if (role == "manufacturer") {
        show_order_list_with_role("manufacturer", 6);
    } else if (role == "accountant") {
        show_order_list_with_role("accountant", 7);
    } else if (role == "seller"){
        // qDebug()<<"---------"<<role;
        show_order_list_with_role("seller", 1);
    } else if (role == "manager") {
        show_order_list_with_role("manager", 4); // editable column index for manager
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
    // We need at least 16 elements (index 0–15) for all columns used
    if (order.size() < 16) {
        qDebug() << "❌ Error: order list size too small:" << order.size();
        return;
    }

    // Sr No
    auto *srItem = new QTableWidgetItem(QString::number(row + 1));
    srItem->setFlags(srItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 0, srItem);

    // User ID, Party ID, Job No
    for (int col = 0; col <= 2; ++col) {
        auto *item = new QTableWidgetItem(order[col].toString());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->orderListTableWidget->setItem(row, col + 1, item);
    }

    QString jobNo = order[2].toString();

    // Order Date (index 7 → column 8)
    auto *orderDateItem = new QTableWidgetItem(order[7].toString());
    orderDateItem->setFlags(orderDateItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 8, orderDateItem);

    // Delivery Date (index 8 → column 9)
    auto *deliveryDateItem = new QTableWidgetItem(order[8].toString());
    deliveryDateItem->setFlags(deliveryDateItem->flags() & ~Qt::ItemIsEditable);
    ui->orderListTableWidget->setItem(row, 9, deliveryDateItem);

    // Job Sheet Button
    auto *jobSheetBtn = new QPushButton("Job Sheet");
    connect(jobSheetBtn, &QPushButton::clicked, this, [=]() {
        openJobSheet(jobNo);
    });
    ui->orderListTableWidget->setCellWidget(row, 10, jobSheetBtn);

    // Print Button
    auto *printBtn = new QPushButton("Print");
    connect(printBtn, &QPushButton::clicked, this, [=]() {
        printJobSheet(jobNo);
    });
    ui->orderListTableWidget->setCellWidget(row, 11, printBtn);

    // Show Image Button
    auto *showImgBtn = new QPushButton("Show Img");
    connect(showImgBtn, &QPushButton::clicked, this, [=]() {
        QString imagePath = order[9].toString();
        if (QFileInfo(imagePath).isRelative()) {
            imagePath = QCoreApplication::applicationDirPath() + "/" + imagePath;
        }
        QPixmap pixmap(imagePath);
        if (pixmap.isNull()) {
            QMessageBox::warning(this, "Image Error", "⚠️ Failed to load image:\n" + imagePath);
            return;
        }

        auto *label = new QLabel;
        label->setPixmap(pixmap.scaled(600, 600, Qt::KeepAspectRatio));
        label->setWindowTitle("Image Preview: " + jobNo);
        label->setAttribute(Qt::WA_DeleteOnClose);
        label->show();
    });
    ui->orderListTableWidget->setCellWidget(row, 12, showImgBtn);

    // Approval & Notes (indices 10–15 → columns 13–18)
    int approvalColStart = 13;
    for (int i = 0; i < 6; ++i) {
        ui->orderListTableWidget->setItem(
            row, approvalColStart + i,
            new QTableWidgetItem(order[10 + i].toString())
            );
    }
}

void OrderList::onTableRightClick(const QPoint &pos)
{
    QModelIndex index = ui->orderListTableWidget->indexAt(pos);
    if (!index.isValid())
        return;

    int row = index.row();
    const int jobNoColumn = 3;  // "Job No" column from your headers

    // Fetch Job No safely
    QString jobNo;
    if (QTableWidgetItem *jobNoItem = ui->orderListTableWidget->item(row, jobNoColumn)) {
        jobNo = jobNoItem->text().trimmed();
    }

    // Build context menu
    QMenu contextMenu(this);
    QAction *jobSheetAction = contextMenu.addAction("Show Job Sheet");

    // (future-proof) – add more actions here if needed
    // QAction *printAction = contextMenu.addAction("Print");
    // QAction *imageAction = contextMenu.addAction("View Image");

    QAction *selectedAction = contextMenu.exec(ui->orderListTableWidget->viewport()->mapToGlobal(pos));
    if (selectedAction == jobSheetAction) {
        if (jobNo.isEmpty()) {
            QMessageBox::warning(this, "Missing Data", "⚠️ Job No is empty.");
            return;
        }
        openJobSheet(jobNo);
    }
}

void OrderList::openJobSheet(const QString &jobNo) {
    JobSheet *sheet = new JobSheet(this, jobNo, userRole);
    sheet->setAttribute(Qt::WA_DeleteOnClose);  // auto-delete on close
    sheet->exec();  // still modal
}

// Cleaned-up and modularized version of printJobSheet
void OrderList::handleAxException(int code, const QString &source, const QString &desc, const QString &help) {
    qDebug() << "COM Exception: Code=" << code << ", Source=" << source << ", Desc=" << desc << ", Help=" << help;
    QMessageBox::critical(this, "Error", QString("Excel operation failed: %1\nCode: %2").arg(desc).arg(code));
}

void OrderList::printJobSheet(const QString &jobNo) {

}

void OrderList::setupStatusCombo(int row, int col, const QString &role, const QString &currentStatus,
                                 const QString &jobNo, const QVariantList &order, const int &editableStatusCol)
{
    QComboBox *combo = new QComboBox(ui->orderListTableWidget);

    QString managerStatus      = order[3].toString();
    QString designerStatus     = order[4].toString();
    QString manufacturerStatus = order[5].toString();

    QStringList possibleStates = getStatusOptions(role);
    QStringList allowedStates;
    int currentIndex = possibleStates.indexOf(currentStatus);

    // ✅ Always show all previous states
    for (int i = 0; i <= currentIndex && i < possibleStates.size(); ++i) {
        allowedStates << possibleStates[i];
    }

    // ✅ Add valid forward states
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
        allowedStates = possibleStates;  // Other roles → full states
    }

    allowedStates.removeDuplicates();
    combo->addItems(allowedStates);
    combo->setCurrentText(currentStatus);

    // --- Apply color function ---
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

    // --- Permission check ---
    bool allowEdit = (role == "manager") ||
                     (role == "designer" && managerStatus == "Order Checked") ||
                     (role == "manufacturer" && managerStatus == "Bagging") ||
                     (role == "accountant" && managerStatus == "QC Done");
    // qDebug()<<allowEdit;
    if (!allowEdit) {
        combo->setEnabled(false);
        QString reason;
        if (role == "designer")      reason = "Manager has not yet Order Checked this design.";
        else if (role == "manufacturer") reason = "Designer must complete their work first.";
        else if (role == "accountant")   reason = "Manufacturer must complete the job first.";
        else if (role == "manager")      reason = "This is not manager’s current stage.";
        combo->setToolTip(reason);
    } else {
        connect(combo, &QComboBox::currentTextChanged, this,
                [=, currentStatusCopy = currentStatus](const QString &newStatus) mutable {

                    QStringList statusOrder = getStatusOptions(role);
                    int oldIndex = statusOrder.indexOf(currentStatusCopy);
                    int newIndex = statusOrder.indexOf(newStatus);

                    // ❌ Prevent backward transition → request admin approval
                    if (newIndex < oldIndex) {
                        QMessageBox::StandardButton reply = QMessageBox::question(
                            this,
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
                            QString note = QInputDialog::getText(
                                this, "Request Note",
                                "Optionally enter a note for this request:",
                                QLineEdit::Normal, "");

                            if (DatabaseUtils::insertStatusChangeRequest(jobNo, userId,
                                                                         currentStatusCopy, newStatus,
                                                                         role, note)) {
                                QMessageBox::information(this, "Request Sent", "Your request has been recorded.");
                            }
                        }
                        return;
                    }

                    // ✅ Apply color
                    applyColor(newStatus);
                    bool allowStatusChange = true;

                    // --- Manager-specific approvals ---
                    auto askApproval = [&](const QString &label, const QString &status) -> bool {
                        int reply = QMessageBox::question(this, "Approve " + label + "?",
                                                          "Do you approve the " + label + "?",
                                                          QMessageBox::Yes | QMessageBox::No);
                        bool approved = (reply == QMessageBox::Yes);
                        QString note;
                        if (!approved) {
                            note = QInputDialog::getText(this, "Rejection Note", "Please enter reason:");
                        }
                        DatabaseUtils::approveStatusChange(jobNo, role, status, approved, note);
                        return approved;
                    };

                    if (role == "manager") {
                        if (newStatus == "Order Checked") {
                            allowStatusChange = askApproval("Order Check", "Order Checked");
                        } else if (newStatus == "Design Checked") {
                            allowStatusChange = askApproval("Design Check", "Design Checked");
                        } else if (newStatus == "QC Done") {
                            allowStatusChange = askApproval("Quality Check", "QC Done");
                        }
                    }

                    // ✅ Generic DB update
                    if (allowStatusChange) {
                        if (DatabaseUtils::updateRoleStatus(jobNo, role, newStatus)) {
                            currentStatusCopy = newStatus;
                        }
                    }

                    // Refresh UI (can optimize to row-only if needed)
                    show_order_list_with_role(userRole, editableStatusCol);
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
        { "designer",      {4, 6, 7, 13, 14, 15, 16, 18} },
        { "manufacturer",  { 4, 5, 7, 12, 13, 14, 15, 16, 17} },
        { "accountant",    {4, 5, 6, 12, 13, 14, 15, 16, 17, 18} },
        { "seller",        {10, 11, 12, 13, 14, 15, 17, 18} },
        { "manager",       {12, 13, 14, 15} }
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

    ui->orderListTableWidget->clearContents();
    ui->orderListTableWidget->setRowCount(orderList.size());

    hideIrrelevantColumns(role);

    for (int row = 0; row < orderList.size(); ++row) {
        const QVariantList &order = orderList[row];

        if (!shouldShowRow(role, order)) {
            ui->orderListTableWidget->setRowHidden(row, true);
            continue;
        }

        populateCommonOrderRow(row, order);
        QString jobNo = order[2].toString();

        // Assuming order[3..6] correspond to status columns
        for (int col = 4; col <= 7; ++col) {
            QString currentStatus = order[col-1].toString();

            if (col == editableStatusCol) {
                // ✅ Use correct col (not col+1)
                setupStatusCombo(row, col, role, currentStatus, jobNo, order, editableStatusCol);
            } else {
                QTableWidgetItem *item = new QTableWidgetItem(currentStatus);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                ui->orderListTableWidget->setItem(row, col, item);
            }
        }
    }

    ui->orderListTableWidget->resizeColumnsToContents();
}
