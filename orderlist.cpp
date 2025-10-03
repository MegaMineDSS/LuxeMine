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
#include <QPainter>
// #include <QPrinter>

// #include <QAxObject>
#include <QInputDialog>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "jobsheet.h"

// #include "header/xlsxdocument.h"

// using namespace QXlsx;
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

void OrderList::drawRow(QPainter &painter, int x, int y, const QVector<int> &widths, int height) {
    for (int w : widths) {
        painter.drawRect(x, y, w, height);
        x += w; // move to next column
    }
}

// Do NOT repeat the default argument here
void OrderList::drawTextRow(QPainter &painter, int x, int y, const QVector<QString> &texts, const QVector<int> &widths)
{
    int curX = x;
    for (int i = 0; i < texts.size(); ++i) {
        painter.drawText(curX, y, texts[i]);
        if (!widths.isEmpty() && i < widths.size()) {
            curX += widths[i];
            // qDebug() << widths[i];
        } else {
            curX += 150; // default spacing if widths not given
        }
    }
}

void OrderList::printJobSheet(const QString &jobNo) {

    // Create "pdfs" folder inside the application directory
    QString pdfDir = QCoreApplication::applicationDirPath() + "/pdfs";
    QDir().mkpath(pdfDir);

    // Generate PDF path with job number
    QString pdfPath = QDir::toNativeSeparators(pdfDir + "/jobSheet_" + jobNo + ".pdf");

    // Remove old file if it exists
    QFile::remove(pdfPath);

    // Set up QPdfWriter for PDF output
    QPdfWriter writer(pdfPath);

    // writer.setPageSize(QPageSize(QPageSize::A4));
    // Set page size to 21 cm × 29.7 cm
    // const qreal cm_to_pt = 72.0 / 2.54;
    QSizeF pageSize(612, 842); // Width × Height in points
    QPageSize customPageSize(pageSize, QPageSize::Point, "CustomA4");
    writer.setPageSize(customPageSize);

    // Zero margins
    QMarginsF margins(0, 0, 0, 0); // left, top, right, bottom
    QPageLayout layout(customPageSize, QPageLayout::Portrait, margins);
    writer.setPageLayout(layout);

    writer.setResolution(400);  // High resolution for print quality

    QPainter painter(&writer);

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(Qt::black, 7);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // Black lines
    painter.setBrush(Qt::black);
    painter.drawRect(2125, 320, 8, 1545);
    painter.drawRect(100, 645, 2025, 8);
    painter.drawRect(100, 1240, 2025, 8);
    painter.drawRect(100, 1670, 2025, 8);
    painter.setBrush(Qt::NoBrush);

    // Right Side default Rectangle
    painter.drawRect(2133, 320, 700, 350);
    painter.drawRect(2833, 320, 442, 65);
    painter.drawRect(2833, 385, 442, 285);
    painter.drawRect(2133, 670, 1142, 1130);

    // Left Side Rectangles
    int y_rect = 320 - 65; //this is -65 because of we are adding 65 in for loop

    // First rows
    // y_rect += 65;
    // drawRow(painter, 100, y_rect, {275, 1200, 275, 275}, 65);


    // Next 2 rows
    for (int i = 0; i < 3; i++) {
        y_rect += 65;
        drawRow(painter, 100, y_rect, {275, 750, 250, 250, 250, 250}, 65);
    }
    // Next 2 rows
    for (int i = 0; i < 2; i++) {
        y_rect += 65;
        drawRow(painter, 100, y_rect, {275, 250, 250, 250, 250, 250, 250, 250}, 65);
    }

    y_rect -= 20;
    // Gold issue table (7 rows)
    for (int i = 0; i < 7; i++) {
        // if(i==0){painter.setBrush(QColor("#60e6eb"));} else {painter.setBrush(Qt::NoBrush);}
        y_rect += 85;
        drawRow(painter, 100, y_rect, {400, 325, 325, 325, 325, 325}, 85);
    }
    painter.setBrush(Qt::NoBrush);  //#60e6eb
    // Next section
    y_rect += 85;  // consistent height step
    drawRow(painter, 100, y_rect, {275, 275, 925, 275, 275}, 90);

    y_rect += 5;
    // 4 rows of equal-height cells
    for (int i = 0; i < 4; i++) {
        y_rect += 85;
        drawRow(painter, 100, y_rect, {375, 275, 275, 275, 275, 275, 275}, 85);
    }

    // Final block (3-row merged cells)
    y_rect += 85; // move down before block
    drawRow(painter, 100, y_rect, {460, 275, 275, 555, 460}, 65 * 3);

    // Sub-rows inside merged block
    int y_sub = y_rect;  // start at same top as merged block
    for (int i = 0; i < 2; i++) {
        y_sub += 65;
        drawRow(painter, 100 + 460, y_sub, {275, 275, 280, 275}, 65);
    }

    // Last section (right side small cols)
    y_rect += 65 * 2; // move below merged block
    drawRow(painter, 2133, y_rect, {190, 190, 190, 190, 190, 190}, 65);


    // Define font for header
    // Company header
    painter.setFont(QFont("Arial", 7, QFont::Bold));
    drawTextRow(painter, 800, 300, {"SHREE LAXMINARAYAN EXPORT"});
    drawTextRow(painter, 2500, 300, {"GST - 24AEXFS9858P1ZI"});

    // Labels font
    painter.setFont(QFont("Arial", 7));

    int y_text = 370;

    // Row 1
    drawTextRow(painter, 110, y_text, {"Job Issue", "Order Date", "Delivery Date", "Note"},
                {1025, 500, 1208, 700});

    // Row 2
    y_text += 68;
    drawTextRow(painter, 110, y_text, {"Party Name", "Party Code", "Order No."},
                {1025, 500, 500});

    // Row 3
    y_text += 63;
    drawTextRow(painter, 110, y_text, {"Item Design", "Design No.", "Job No."},
                {1025, 500, 500});

    // Metal info row
    y_text += 67;
    drawTextRow(painter, 110, y_text,
                {"Metal Name", "Met. Purity", "Met. Color", "Size No.", "MM", "Length", "Width", "Height"},
                {275, 250, 250, 250, 250, 250, 250, 250});

    // Gold Issue Table header (bold)
    y_text += 143;
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    drawTextRow(painter, 510, y_text, {"Issue Wt.", "Ret. Dust Wt.", "Loss Wt.", "Return Wt.", "Loss %"},
                {325, 325, 325, 325, 325});

    // Left side job process rows
    // painter.setFont(QFont("Arial", 7));
    y_text += 87;
    drawTextRow(painter, 110, y_text, {"Filing"});

    y_text += 87;
    drawTextRow(painter, 110, y_text, {"Buffing"});

    y_text += 87;
    drawTextRow(painter, 110, y_text, {"Free Polich"});

    y_text += 85;
    drawTextRow(painter, 110, y_text, {"Setting"});

    y_text += 85;
    drawTextRow(painter, 110, y_text, {"Final Poliching"});

    y_text += 83;
    drawTextRow(painter, 110, y_text, {"Total"});

    // Diamond & Stone Issue header
    y_text += 90;
    drawTextRow(painter, 110 + 275 + 275 + 180, y_text, {"Diamond & Stone Issue"});

    painter.setFont(QFont("Arial", 8.5, QFont::Bold));
    y_text += 85;
    drawTextRow(painter, 110 + 375, y_text,
                {"Issue Pcs.", "Issue Wt.", "Return Pcs.", "Return Wt.", "Broken Pcs.", "Broken Wt."},
                {275, 275, 275, 275, 275, 275});

    // Diamond / Stone / Other labels
    // painter.setFont(QFont("Arial", 7));
    y_text += 85;
    drawTextRow(painter, 110, y_text, {"Diamond"});

    y_text += 85;
    drawTextRow(painter, 110, y_text, {"Stone"});

    y_text += 85;
    drawTextRow(painter, 110, y_text, {"Other"});

    // Small footer text
    y_text += 53;
    painter.setFont(QFont("Arial", 5.5));
    drawTextRow(painter, 110, y_text, {"For. Shree Laxminarayan Export"});

    // Overleaf text with word wrap
    painter.drawText(QRect(110 + 460 + 275 + 275 + 555, y_text - 25, 460, 1000),
                     Qt::TextWordWrap,
                     "Received the above goods as per conditions overleaf.");

    painter.setFont(QFont("Arial", 7));
    y_text += 20;

    // Weight section
    int x_text = 110 + 460;
    drawTextRow(painter, x_text, y_text, {"Diamond Wt.", "Final Product Wt."}, {275+275+110, 660});

    y_text += 62;
    drawTextRow(painter, x_text, y_text, {"Stone Wt.", "Net. Wt."}, {275+275, 525});

    y_text += 65;
    drawTextRow(painter, x_text, y_text, {"Other Wt.", "Gross. Wt."}, {275+275, 525});

    // Rightmost labels
    drawTextRow(painter, x_text + 275 + 275 + 555 + 460 + 8, y_text, {"Diamond", "Stone", "Other"}, {190+190, 190+190, 190});

    painter.setFont(QFont("Arial", 5.5));
    // Signatures
    x_text -= 460;
    drawTextRow(painter, x_text, y_text, {"Proprietor/ Authorised Signature"});
    drawTextRow(painter, x_text + 460 + 275 + 275 + 555, y_text, {"Receiver's Signature"});


















    painter.end();  // Finish painting

    if (QFile::exists(pdfPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
    } else {
        QMessageBox::warning(this, "Error", "Failed to create PDF file: " + pdfPath);
    }

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
