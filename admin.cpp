#include "admin.h"
#include "ui_admin.h"

#include <QMessageBox>
#include <QPixmap>
#include <QSqlQuery>
#include <QSqlError>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QInputDialog>

#include "readonlydelegate.h"
#include "databaseutils.h"
#include "utils.h"
#include "PdfListDialog.h"


Admin::Admin(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Admin)
    , roundDiamondModel(nullptr)
    , fancyDiamondModel(nullptr)
    , currentIndex(0)
{
    ui->setupUi(this);
    setWindowSize(this);
    ui->stackedWidget->setCurrentIndex(0);

    // Ensure the window behaves like a top-level window
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowTitle("Admin");                     // Set the window title
    setWindowIcon(QIcon(":/icon/user.png")); // Set the window icon
    ui->name_lineEdit->setFocus();               // focus on name
}

Admin::~Admin()
{
    delete roundDiamondModel;
    delete fancyDiamondModel;
    delete ui;
}

void Admin::setRequestedPage(int index)
{
    requestedPageIndex = index;

    if (ui->stackedWidget->currentIndex() == 1) {
        switch (requestedPageIndex) {
        case 0: on_show_images_clicked(); break;
        case 1: on_update_price_clicked(); break;
        case 2: on_add_dia_clicked(); break;
        case 3: on_show_users_clicked(); break;
        case 4: on_jewelry_menu_button_clicked(); break;
        case 5: on_orderBookUsersPushButton_clicked(); break;
        case 6: on_orderBookRequestPushButton_clicked(); break;
        default: on_show_images_clicked(); break;
        }
        requestedPageIndex = -1; // reset
    }
}

void Admin::on_show_images_clicked()
{
    ui->Admin_panel->setCurrentIndex(0);
    imagePaths = DatabaseUtils::fetchImagePaths();
    currentIndex = 0;

    auto updateImage = [this]() {
        if (!imagePaths.isEmpty()) {
            QPixmap pixmap(imagePaths[currentIndex]);
            ui->image_viewer->setPixmap(pixmap.scaled(ui->image_viewer->size(),
                                                      Qt::KeepAspectRatio,
                                                      Qt::SmoothTransformation));
        } else {
            ui->image_viewer->clear();
        }
    };

    if (imagePaths.isEmpty()) {
        QMessageBox::warning(this, "No Images", "No images found in the database.");
        return;
    }

    updateImage();

    disconnect(ui->next_image, nullptr, nullptr, nullptr);
    disconnect(ui->prev_image, nullptr, nullptr, nullptr);

    connect(ui->next_image, &QPushButton::clicked, this, [this, updateImage]() {
        if (!imagePaths.isEmpty()) {
            currentIndex = (currentIndex + 1) % imagePaths.size();
            updateImage();
        }
    });

    connect(ui->prev_image, &QPushButton::clicked, this, [this, updateImage]() {
        if (!imagePaths.isEmpty()) {
            currentIndex = (currentIndex - 1 + imagePaths.size()) % imagePaths.size();
            updateImage();
        }
    });
}

void Admin::on_update_price_clicked()
{
    ui->Admin_panel->setCurrentIndex(1);
}

void Admin::on_add_dia_clicked()
{
    ui->Admin_panel->setCurrentIndex(2);
}

void Admin::set_comboBox_role()
{
    QStringList roles = DatabaseUtils::fetchRoles();
    ui->roleComboBox->clear();
    ui->roleComboBox->addItems(roles);
}

void Admin::on_orderBookUsersPushButton_clicked()
{
    ui->Admin_panel->setCurrentIndex(5);
    set_comboBox_role();
}

void Admin::handleStatusChangeApproval(int requestId, bool approved, int rowInTable, const QString& note)
{
    if (!DatabaseUtils::updateStatusChangeRequest(requestId, approved, note)) {
        QMessageBox::critical(this, "Error", "Failed to update status change request.");
        return;
    }

    // DB changes successful. Clearing request fields in UI.
    for (int col = 7; col <= 13; ++col) {
        QWidget* widget = ui->jobsheet_request_table->cellWidget(rowInTable, col);
        if (widget) {
            delete widget;
            ui->jobsheet_request_table->setCellWidget(rowInTable, col, nullptr);
        } else {
            QTableWidgetItem* item = ui->jobsheet_request_table->item(rowInTable, col);
            if (!item){
                item = new QTableWidgetItem();
                ui->jobsheet_request_table->setItem(rowInTable, col, item);
            }
            item->setText("");
        }
    }
}

void Admin::onRoleStatusChanged(const QString &jobNo, const QString &fieldName, const QString &newStatus)
{
    if (!DatabaseUtils::updateRoleStatus(jobNo, fieldName, newStatus)) {
        QMessageBox::warning(this, "Update Failed", "Could not update role status.");
    }
}

void Admin::on_orderBookRequestPushButton_clicked()
{
    ui->Admin_panel->setCurrentIndex(6);

    // Clear old table contents (deletes old widgets)
    ui->jobsheet_request_table->clearContents();
    ui->jobsheet_request_table->setRowCount(0);

    ui->jobsheet_request_table->setColumnCount(14);
    ui->jobsheet_request_table->setHorizontalHeaderLabels({
        "Seller ID", "Party ID", "Job No", "Manager", "Designer", "Manufacturer", "Accountant",
        "Request ID", "Request Role", "Request Role ID", "From", "To", "Request Time", "Action"
    });

    QList<JobSheetRequest> rows = DatabaseUtils::fetchJobSheetRequests();
    int row = 0;

    for (const auto &r : rows) {
        ui->jobsheet_request_table->insertRow(row);

        // Static columns
        for (int col = 0; col <= 2; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(
                (col == 0) ? r.sellerId :
                    (col == 1) ? r.partyId : r.jobNo
                );
            item->setTextAlignment(Qt::AlignCenter);
            ui->jobsheet_request_table->setItem(row, col, item);
        }

        // Combo setup helper
        auto addCombo = [&](int col, const QStringList &items, const QString &current) -> QComboBox* {
            QComboBox *combo = new QComboBox(ui->jobsheet_request_table); // parent set
            combo->setStyleSheet(R"(
            QComboBox {
                background-color: #F3F3F3;
                border: 1px solid #B4B4B4;
                border-radius: 2px;
                color: #2C2C2C;
                font-size: 12px;

            }

            QComboBox:focus {
                border: 1px solid #305C91;
                background-color: #FFFFFF;
            }

            QComboBox:hover {
                border: 1px solid #888;
            }

            QComboBox::down-arrow {
                image: url(":/icon/expand-arrow.png");
                width: 10px;
                height: 10px;
                padding-right: 2px;
            }

            QComboBox::drop-down {
                border: none;
                margin-right: 4px;
            }

            QComboBox QAbstractItemView {
                background-color: #FFFFFF;
                border: 1px solid #B4B4B4;
                selection-background-color: #305C91;
                selection-color: #FFFFFF;
                font-size: 12px;

            }
            )");
            combo->view()->setMinimumWidth(combo->sizeHint().width() + 50);
            combo->addItems(items);
            combo->setCurrentText(current);
            ui->jobsheet_request_table->setCellWidget(row, col, combo);
            return combo;
        };

        QComboBox *managerCombo      = addCombo(3, {"Pending", "Order Checked", "Design Checked", "RPD", "Casting", "Bagging", "QC Done"}, r.manager);
        QComboBox *designerCombo     = addCombo(4, {"Pending", "Working", "Completed"}, r.designer);
        QComboBox *manufacturerCombo = addCombo(5, {"Pending", "Working", "Completed"}, r.manufacturer);
        QComboBox *accountantCombo   = addCombo(6, {"Pending", "Working", "Completed"}, r.accountant);

        // Status change lambda
        auto connectStatusChange = [=, this]() {
            QString managerStatus = managerCombo->currentText();
            QString designerStatus = designerCombo->currentText();
            QString manufacturerStatus = manufacturerCombo->currentText();
            QString accountantStatus = accountantCombo->currentText();

            if (managerStatus == "Pending") {
                onRoleStatusChanged(r.jobNo, "Designer", "Pending");
                onRoleStatusChanged(r.jobNo, "Manufacturer", "Pending");
                onRoleStatusChanged(r.jobNo, "Accountant", "Pending");
                designerCombo->setCurrentText("Pending");
                manufacturerCombo->setCurrentText("Pending");
                accountantCombo->setCurrentText("Pending");
                designerCombo->setEnabled(false);
                manufacturerCombo->setEnabled(false);
                accountantCombo->setEnabled(false);
            } else if (managerStatus == "Order Checked") {
                onRoleStatusChanged(r.jobNo, "Manufacturer", "Pending");
                onRoleStatusChanged(r.jobNo, "Accountant", "Pending");
                manufacturerCombo->setCurrentText("Pending");
                accountantCombo->setCurrentText("Pending");
                designerCombo->setEnabled(true);
                manufacturerCombo->setEnabled(false);
                accountantCombo->setEnabled(false);
            } else if(managerStatus == "Design Checked" || managerStatus == "RPD" || managerStatus == "Casting"){
                onRoleStatusChanged(r.jobNo, "Designer", "Completed");
                onRoleStatusChanged(r.jobNo, "Manufacturer", "Pending");
                onRoleStatusChanged(r.jobNo, "Accountant", "Pending");
                designerCombo->setCurrentText("Completed");
                manufacturerCombo->setCurrentText("Pending");
                accountantCombo->setCurrentText("Pending");
                designerCombo->setEnabled(false);
                manufacturerCombo->setEnabled(false);
                accountantCombo->setEnabled(false);
            } else if (managerStatus == "Bagging") {
                onRoleStatusChanged(r.jobNo, "Designer", "Completed");
                onRoleStatusChanged(r.jobNo, "Accountant", "Pending");
                designerCombo->setCurrentText("Completed");
                accountantCombo->setCurrentText("Pending");
                designerCombo->setEnabled(false);
                manufacturerCombo->setEnabled(true);
                accountantCombo->setEnabled(false);
            } else if (managerStatus == "QC Done") {
                onRoleStatusChanged(r.jobNo, "Designer", "Completed");
                onRoleStatusChanged(r.jobNo, "Manufacturer", "Completed");
                designerCombo->setCurrentText("Completed");
                manufacturerCombo->setCurrentText("Completed");
                designerCombo->setEnabled(false);
                manufacturerCombo->setEnabled(false);
                accountantCombo->setEnabled(true);
            }

            onRoleStatusChanged(r.jobNo, "Manager", managerStatus);
        };

        connect(managerCombo,      &QComboBox::currentTextChanged, this, [=](const QString &){ connectStatusChange(); });
        connect(designerCombo,     &QComboBox::currentTextChanged, this, [=](const QString &){ connectStatusChange(); });
        connect(manufacturerCombo, &QComboBox::currentTextChanged, this, [=](const QString &){ connectStatusChange(); });
        connect(accountantCombo,   &QComboBox::currentTextChanged, this, [=](const QString &){ connectStatusChange(); });

        // Fill cols 7–12
        ui->jobsheet_request_table->setItem(row, 7,  new QTableWidgetItem(QString::number(r.requestId)));
        ui->jobsheet_request_table->setItem(row, 8,  new QTableWidgetItem(r.requestRole));
        ui->jobsheet_request_table->setItem(row, 9,  new QTableWidgetItem(r.requestRoleId));
        ui->jobsheet_request_table->setItem(row, 10, new QTableWidgetItem(r.fromStatus));
        ui->jobsheet_request_table->setItem(row, 11, new QTableWidgetItem(r.toStatus));
        ui->jobsheet_request_table->setItem(row, 12, new QTableWidgetItem(r.requestTime));

        // Action buttons
        if (r.requestId > 0) {
            QWidget *actionWidget = new QWidget(ui->jobsheet_request_table); // parent
            QHBoxLayout *layout = new QHBoxLayout(actionWidget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignCenter);

            QPushButton *approveButton = new QPushButton("✓", actionWidget);
            QPushButton *rejectButton  = new QPushButton("✕", actionWidget);

            approveButton->setStyleSheet(R"(                QPushButton {                    background-color: #F3F3F3;                    border: 1px solid #2A4E7C;                    border-radius: 2px;                    color: #2C2C2C;                    font-size: 14px;                    font-weight: 600;                    padding: 6px 14px;                   min-width: 50px;                }                                QPushButton:hover {                    background-color: #6CDA69;                }                                QPushButton:pressed {                    background-color: #DADADA;                    border: 1px solid #666;                }                                QPushButton:disabled {                    background-color: #F0F0F0;                    color: #A0A0A0;                    border: 1px solid #D0D0D0;                }                            )");
            rejectButton->setStyleSheet(R"(                QPushButton {                    background-color: #F3F3F3;                    border: 1px solid #2A4E7C;                    border-radius: 2px;                    color: #2C2C2C;                    font-size: 14px;                    font-weight: 600;                    padding: 6px 14px;                   min-width: 50px;                }                                QPushButton:hover {                    background-color: #D85D5D;                }                                QPushButton:pressed {                    background-color: #DADADA;                    border: 1px solid #666;                }                                QPushButton:disabled {                    background-color: #F0F0F0;                    color: #A0A0A0;                    border: 1px solid #D0D0D0;                }                            )");

            connect(approveButton, &QPushButton::clicked, this, [=, this]() {
                handleStatusChangeApproval(r.requestId, true, row);
            });

            connect(rejectButton, &QPushButton::clicked, this, [=, this]() {
                bool ok = false;
                QString reason = QInputDialog::getText(this, "Rejection Note", "Please enter a reason (optional):", QLineEdit::Normal, QString(), &ok);

                if (ok) {
                    handleStatusChangeApproval(r.requestId, false, row, reason);
                }
            });

            layout->addWidget(approveButton);
            layout->addWidget(rejectButton);
            ui->jobsheet_request_table->setCellWidget(row, 13, actionWidget);
        }

        ++row;
    }
}

void Admin::on_show_users_clicked()
{
    ui->Admin_panel->setCurrentIndex(3);

    // Ensure context menu connection is only made once
    static bool contextMenuConnected = false;
    if (!contextMenuConnected) {
        ui->tableWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(ui->tableWidget_2, &QTableWidget::customContextMenuRequested, this,
                [this](const QPoint &pos) {
                    QTableWidgetItem *item = ui->tableWidget_2->itemAt(pos);
                    if (!item) return;

                    int row = ui->tableWidget_2->rowAt(pos.y());
                    if (row < 0) return;

                    QString userId = ui->tableWidget_2->item(row, 0)->text();

                    QMenu contextMenu(tr("Context Menu"), this);
                    QAction *deleteAction = contextMenu.addAction("Delete User");
                    QAction *showPdfsAction = contextMenu.addAction("Show PDFs");

                    connect(deleteAction, &QAction::triggered, this, &Admin::on_deleteUser_triggered);
                    connect(showPdfsAction, &QAction::triggered, this, [this, userId]() {
                        QList<PdfRecord> pdfRecords = DatabaseUtils::getUserPdfs(userId);
                        if (pdfRecords.isEmpty()) {
                            QMessageBox::information(this, "No PDFs",
                                                     QString("No PDFs found for user: %1").arg(userId));
                            return;
                        }

                        // Create on stack -> no leak
                        PdfListDialog dialog(userId, pdfRecords, this);
                        dialog.exec();
                    });

                    contextMenu.exec(ui->tableWidget_2->viewport()->mapToGlobal(pos));
                });
        contextMenuConnected = true;
    }

    // Save current column widths
    QVector<int> columnWidths;
    for (int col = 0; col < ui->tableWidget_2->columnCount(); ++col)
        columnWidths.append(ui->tableWidget_2->columnWidth(col));

    // Clear just the cell contents but keep headers intact
    ui->tableWidget_2->clearContents();
    ui->tableWidget_2->setRowCount(0);

    // If you want to set headers only once, move this to your constructor/setup:
    ui->tableWidget_2->setColumnCount(3);
    ui->tableWidget_2->setHorizontalHeaderLabels({"User ID", "Company Name", "Last PDF Path"});
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);


    // Fetch data
    QList<QVariantList> userDetails = DatabaseUtils::fetchUserDetailsForAdmin();
    if (userDetails.isEmpty()) {
        QMessageBox::warning(this, "No Users", "No user data found in the database.");
        return;
    }

    // Populate rows
    for (const QVariantList &row : userDetails) {
        int rowIndex = ui->tableWidget_2->rowCount();
        ui->tableWidget_2->insertRow(rowIndex);

        for (int col = 0; col < row.size(); ++col) {
            QString displayText = row[col].toString();
            if (col == 2 && displayText.startsWith("pdfs/")) {
                displayText = displayText.mid(5); // strip "pdfs/"
            }
            QTableWidgetItem *item = new QTableWidgetItem(displayText);
            item->setTextAlignment(Qt::AlignCenter);

            if (col == 2)
                item->setData(Qt::UserRole, row[col].toString()); // store full path

            ui->tableWidget_2->setItem(rowIndex, col, item);
        }
    }

    // Restore column widths
    if (columnWidths.size() == ui->tableWidget_2->columnCount()) {
        for (int col = 0; col < ui->tableWidget_2->columnCount(); ++col)
            ui->tableWidget_2->setColumnWidth(col, columnWidths[col]);
    } else {
        ui->tableWidget_2->setColumnWidth(0, 150);
        ui->tableWidget_2->setColumnWidth(1, 200);
        ui->tableWidget_2->setColumnWidth(2, 250);
    }
}

void Admin::on_logout_clicked()
{
    if (parentWidget())
    {
        parentWidget()->show();
    }

    this->close(); // Close after showing parent
}

void Admin::on_gold_button_clicked()
{
    QMap<QString, QString> goldPrices = DatabaseUtils::fetchGoldPrices();
    if (goldPrices.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Failed to fetch gold prices.");
        return;
    }

    ui->label_10Kt->setText(goldPrices.value("10Kt", ""));
    ui->label_14Kt->setText(goldPrices.value("14Kt", ""));
    ui->label_18Kt->setText(goldPrices.value("18Kt", ""));
    ui->label_20Kt->setText(goldPrices.value("20Kt", ""));
    ui->label_22Kt->setText(goldPrices.value("22Kt", ""));
    ui->label_24Kt->setText(goldPrices.value("24Kt", ""));
    ui->gold_dia_update_price->setCurrentIndex(0);
}

void Admin::on_dia_button_clicked()
{
    ui->gold_dia_update_price->setCurrentIndex(1);
}

void Admin::on_cancel_button_clicked()
{
    if (parentWidget())
        parentWidget()->show();
    close();
}

bool Admin::checkLoginCredentials(const QString &username, const QString &password)
{
    QString role;
    bool valid = DatabaseUtils::checkAdminCredentials(username, password, role);

    if (!valid) {
        QMessageBox::warning(this, "Login Failed", "Invalid username or password.");
    }

    return valid;
}

void Admin::on_login_button_clicked()
{
    QString username = ui->name_lineEdit->text();
    QString password = ui->admin_password_lineEdit->text();

    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "Please enter both username and password.");
        return;
    }

    if (checkLoginCredentials(username, password))
    {
        ui->stackedWidget->setCurrentIndex(1); // show main panel after login

        if (requestedPageIndex == -1) {
            on_show_images_clicked(); // default
        } else {
            switch (requestedPageIndex) {
            case 0: on_show_images_clicked(); break;
            case 1: on_update_price_clicked(); break;
            case 2: on_add_dia_clicked(); break;
            case 3: on_show_users_clicked(); break;
            case 4: on_jewelry_menu_button_clicked(); break;
            case 5: on_orderBookUsersPushButton_clicked(); break;
            case 6: on_orderBookRequestPushButton_clicked(); break;
            default: on_show_images_clicked(); break;
            }
            requestedPageIndex = -1; // reset after use
        }
    }


    ui->name_lineEdit->clear();
    ui->admin_password_lineEdit->clear();
}

void Admin::on_round_dia_button_clicked()
{
    ui->add_dia_stackwidget->setCurrentIndex(0);
}

void Admin::on_fancy_dia_button_clicked()
{
    ui->add_dia_stackwidget->setCurrentIndex(1);
}

void Admin::closeEvent(QCloseEvent *event)
{
    // Closing AdminMenuButtons UI
    if (newAdminMenuButtons && newAdminMenuButtons->isVisible())
    {
        newAdminMenuButtons->close(); // Ensure overlay is closed
    }

    on_logout_clicked();

    event->accept();
}

void Admin::on_AddRoundDiamond_clicked()
{
    QString sieve = ui->sieve_lineEdit->text();
    QString sizeMM = ui->sizeMM_lineEdit_round->text();
    QString weight = ui->Weight_lineEdit_round->text();
    QString price = ui->price_lineEdit_round->text();

    if (sieve.isEmpty() || sizeMM.isEmpty() || weight.isEmpty() || price.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "All fields must be filled!");
        return;
    }

    bool sizeOk, weightOk, priceOk;
    double sizeMMValue = sizeMM.toDouble(&sizeOk);
    double weightValue = weight.toDouble(&weightOk);
    double priceValue = price.toDouble(&priceOk);

    if (!sizeOk || !weightOk || !priceOk)
    {
        QMessageBox::warning(this, "Input Error", "Please ensure all fields contain valid numbers.");
        return;
    }

    if (DatabaseUtils::sizeMMExists("Round_diamond", sizeMMValue))
    {
        QMessageBox::warning(this, "Duplicate Entry", "The sizeMM value already exists. Please use a unique size.");
        return;
    }

    if (DatabaseUtils::insertRoundDiamond(sieve, sizeMMValue, weightValue, priceValue))
    {
        QMessageBox::information(this, "Success", "Round diamond data inserted successfully!");
        ui->sieve_lineEdit->clear();
        ui->sizeMM_lineEdit_round->clear();
        ui->Weight_lineEdit_round->clear();
        ui->price_lineEdit_round->clear();
    }
    else
    {
        QMessageBox::critical(this, "Insert Error", "Failed to insert round diamond data.");
    }
}

void Admin::on_AddFancyDiamond_clicked()
{
    QString shape = ui->shape_lineEdit->text();
    QString sizeMM = ui->size_lineEdit_Fancy->text();
    QString weight = ui->Weight_lineEdit_Fancy->text();
    QString price = ui->price_lineEdit_Fancy->text();

    if (shape.isEmpty() || sizeMM.isEmpty() || weight.isEmpty() || price.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "All fields must be filled!");
        return;
    }

    bool weightOk, priceOk;
    double weightValue = weight.toDouble(&weightOk);
    double priceValue = price.toDouble(&priceOk);

    if (!weightOk || !priceOk)
    {
        QMessageBox::warning(this, "Input Error", "Please ensure weight and price are valid numbers.");
        return;
    }

    if (DatabaseUtils::insertFancyDiamond(shape, sizeMM, weightValue, priceValue))
    {
        QMessageBox::information(this, "Success", "Fancy diamond data inserted successfully!");
        ui->shape_lineEdit->clear();
        ui->size_lineEdit_Fancy->clear();
        ui->Weight_lineEdit_Fancy->clear();
        ui->price_lineEdit_Fancy->clear();
    }
    else
    {
        QMessageBox::critical(this, "Insert Error", "Failed to insert fancy diamond data.");
    }
}

void Admin::on_RoundDiamind_Price_clicked()
{
    ui->DiamondPrice_update_2->setCurrentIndex(0);
    delete roundDiamondModel;
    roundDiamondModel = DatabaseUtils::createTableModel(this, "Round_diamond");
    if (!roundDiamondModel)
    {
        QMessageBox::warning(this, "Error", "Failed to load round diamond table.");
        return;
    }

    ui->tableView->setModel(roundDiamondModel);
    roundDiamondModel->setHeaderData(0, Qt::Horizontal, "Sieve");
    roundDiamondModel->setHeaderData(1, Qt::Horizontal, "Size (MM)");
    roundDiamondModel->setHeaderData(2, Qt::Horizontal, "Weight");
    roundDiamondModel->setHeaderData(3, Qt::Horizontal, "Price");

    for (int col = 0; col < roundDiamondModel->columnCount(); ++col)
    {
        if (col != 3)
        {
            ui->tableView->setItemDelegateForColumn(col, new ReadOnlyDelegate(this));
        }
    }
}

void Admin::on_updateRoundDiamond_price_clicked()
{
    if (!roundDiamondModel || !roundDiamondModel->submitAll())
    {
        QMessageBox::warning(this, "Update Error", "Failed to update round diamond prices.");
        return;
    }
    QMessageBox::information(this, "Success", "Round diamond prices updated successfully!");
}

void Admin::on_FancyDiamond_Price_clicked()
{
    ui->DiamondPrice_update_2->setCurrentIndex(1);

    delete fancyDiamondModel;
    fancyDiamondModel = DatabaseUtils::createTableModel(this, "Fancy_diamond");
    if (!fancyDiamondModel) {
        QMessageBox::warning(this, "Error", "Failed to load fancy diamond table.");
        return;
    }

    ui->tableView_2->setModel(fancyDiamondModel);
    fancyDiamondModel->setHeaderData(0, Qt::Horizontal, "Shape");
    fancyDiamondModel->setHeaderData(1, Qt::Horizontal, "Size (MM)");
    fancyDiamondModel->setHeaderData(2, Qt::Horizontal, "Weight");
    fancyDiamondModel->setHeaderData(3, Qt::Horizontal, "Price");

    // 🔧 Clear any existing delegates before reassigning
    // ui->tableView_2->setItemDelegate(nullptr);

    for (int col = 0; col < fancyDiamondModel->columnCount(); ++col) {
        // qDebug()<<fancyDiamondModel->;
        if (col != 3) {
            ui->tableView_2->setItemDelegateForColumn(col, new ReadOnlyDelegate(ui->tableView_2));
        }
    }
}

void Admin::on_updateFancyDiamond_price_clicked()
{
    if (!fancyDiamondModel || !fancyDiamondModel->submitAll())
    {
        QMessageBox::warning(this, "Update Error", "Failed to update fancy diamond prices.");
        return;
    }
    QMessageBox::information(this, "Success", "Fancy diamond prices updated successfully!");
}

void Admin::on_upadateGold_Price_clicked()
{
    QMap<QString, QString> priceUpdates;
    priceUpdates["10Kt"] = ui->lineEdit_10Kt->text();
    priceUpdates["14Kt"] = ui->lineEdit_14Kt->text();
    priceUpdates["18Kt"] = ui->lineEdit_18Kt->text();
    priceUpdates["20Kt"] = ui->lineEdit_20Kt->text();
    priceUpdates["22Kt"] = ui->lineEdit_22Kt->text();
    priceUpdates["24Kt"] = ui->lineEdit_24Kt->text();

    if (DatabaseUtils::updateGoldPrices(priceUpdates))
    {
        QMessageBox::information(this, "Success", "Gold prices updated successfully!");
        ui->lineEdit_10Kt->clear();
        ui->lineEdit_14Kt->clear();
        ui->lineEdit_18Kt->clear();
        ui->lineEdit_20Kt->clear();
        ui->lineEdit_22Kt->clear();
        ui->lineEdit_24Kt->clear();
        on_gold_button_clicked();
    }
    else
    {
        QMessageBox::warning(this, "Error", "Failed to update gold prices.");
    }
}

void Admin::on_deleteUser_triggered()
{
    // Get the selected row
    int row = ui->tableWidget_2->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, "No Selection", "Please select a user to delete.");
        return;
    }

    // Get user_id from the first column
    QString userId = ui->tableWidget_2->item(row, 0)->text();
    QString companyName = ui->tableWidget_2->item(row, 1)->text();

    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Delete",
        QString("Are you sure you want to delete user %1 (%2)? This will remove all associated records.").arg(userId, companyName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // Delete user from database
        if (DatabaseUtils::deleteUser(userId))
        {
            QMessageBox::information(this, "Success", "User " + userId + " deleted successfully.");
        }
        else
        {
            QMessageBox::critical(this, "Delete Error", "Failed to delete user: " + userId);
        }

        // Refresh the table
        on_show_users_clicked();
    }
}

void Admin::on_tableWidget_2_cellDoubleClicked(int row, int column)
{
    if (column == 2)
    { // Last PDF Path column
        QTableWidgetItem *item = ui->tableWidget_2->item(row, column);
        if (!item)
            return;
        QString pdfPath = item->data(Qt::UserRole).toString();
        if (pdfPath.isEmpty())
        {
            QMessageBox::information(this, "No PDF", "No PDF available for this user.");
            return;
        }
        QFile pdfFile(pdfPath);
        if (!pdfFile.exists())
        {
            QMessageBox::warning(this, "Error", "PDF file does not exist: " + pdfPath);
            return;
        }
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath)))
        {
            QMessageBox::critical(this, "Error", "Failed to open PDF: " + pdfPath);
        }
        else
        {
            qDebug() << "Opened PDF:" << pdfPath;
        }
    }
}

void Admin::on_jewelry_menu_button_clicked()
{
    if (!jewelryMenuTable)
    {
        setupJewelryMenuPage(); // Initialize page only once
    }
    ui->Admin_panel->setCurrentIndex(4);
    populateJewelryMenuTable();
    populateParentCategoryComboBox();
}

void Admin::setupJewelryMenuPage()
{
    QWidget *jewelryMenuPage = ui->Admin_panel->widget(4);
    if (!jewelryMenuPage) {
        qWarning() << "[ERROR] Jewelry menu page (index 4) not found.";
        return;
    }

    // Clear existing layout safely
    if (auto oldLayout = jewelryMenuPage->layout()) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (auto w = item->widget())
                w->deleteLater();
            delete item;
        }
        delete oldLayout;
    }

    auto *mainLayout = new QVBoxLayout(jewelryMenuPage);

    // --- Category input ---
    {
        auto *categoryLayout = new QHBoxLayout();
        auto *categoryLabel = new QLabel("Category Name:", jewelryMenuPage);
        categoryNameLineEdit = new QLineEdit(jewelryMenuPage);
        categoryNameLineEdit->setPlaceholderText("Enter category name");
        addCategoryButton = new QPushButton("Add Category", jewelryMenuPage);

        categoryLayout->addWidget(categoryLabel);
        categoryLayout->addWidget(categoryNameLineEdit);
        categoryLayout->addWidget(addCategoryButton);
        mainLayout->addLayout(categoryLayout);
    }

    // --- Item input ---
    {
        auto *itemFormLayout = new QFormLayout();
        parentCategoryComboBox = new QComboBox(jewelryMenuPage);
        itemNameLineEdit = new QLineEdit(jewelryMenuPage);
        itemNameLineEdit->setPlaceholderText("Enter item name");
        displayTextLineEdit = new QLineEdit(jewelryMenuPage);
        displayTextLineEdit->setPlaceholderText("Enter display text");
        addItemButton = new QPushButton("Add Item", jewelryMenuPage);

        itemFormLayout->addRow("Parent Category:", parentCategoryComboBox);
        itemFormLayout->addRow("Item Name:", itemNameLineEdit);
        itemFormLayout->addRow("Display Text:", displayTextLineEdit);
        itemFormLayout->addRow(addItemButton);
        mainLayout->addLayout(itemFormLayout);
    }

    // --- Table ---
    jewelryMenuTable = new QTableWidget(jewelryMenuPage);
    jewelryMenuTable->setColumnCount(4);
    jewelryMenuTable->setHorizontalHeaderLabels({"ID", "Parent ID", "Name", "Display Text"});
    jewelryMenuTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    jewelryMenuTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    jewelryMenuTable->setSelectionMode(QAbstractItemView::SingleSelection);
    jewelryMenuTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(jewelryMenuTable);

    // --- Delete button ---
    {
        auto *deleteLayout = new QHBoxLayout();
        deleteItemButton = new QPushButton("Delete Item", jewelryMenuPage);
        deleteLayout->addWidget(deleteItemButton);
        deleteLayout->addStretch();
        mainLayout->addLayout(deleteLayout);
    }

    jewelryMenuPage->setLayout(mainLayout);

    // --- Connections (prevent duplicates) ---
    disconnect(addCategoryButton, nullptr, this, nullptr);
    disconnect(addItemButton, nullptr, this, nullptr);
    disconnect(deleteItemButton, nullptr, this, nullptr);

    connect(addCategoryButton, &QPushButton::clicked, this, &Admin::on_add_menu_category_clicked);
    connect(addItemButton, &QPushButton::clicked, this, &Admin::on_add_menu_item_clicked);
    connect(deleteItemButton, &QPushButton::clicked, this, &Admin::on_delete_menu_item_clicked);

    // --- Initialize ---
    populateJewelryMenuTable();
    populateParentCategoryComboBox();
}

void Admin::populateJewelryMenuTable()
{
    if (!jewelryMenuTable)
        return;

    jewelryMenuTable->setRowCount(0);

    const QList<QVariantList> menuItems = DatabaseUtils::fetchJewelryMenuItems();
    for (const QVariantList &item : menuItems)
    {
        const int row = jewelryMenuTable->rowCount();
        jewelryMenuTable->insertRow(row);

        auto *idItem = new QTableWidgetItem(QString::number(item[0].toInt()));
        idItem->setData(Qt::UserRole, item[0].toInt());
        jewelryMenuTable->setItem(row, 0, idItem);

        auto *parentItem = new QTableWidgetItem(item[1].toInt() == -1 ? "None" : QString::number(item[1].toInt()));
        parentItem->setData(Qt::UserRole, item[1].toInt());
        jewelryMenuTable->setItem(row, 1, parentItem);

        jewelryMenuTable->setItem(row, 2, new QTableWidgetItem(item[2].toString()));
        jewelryMenuTable->setItem(row, 3, new QTableWidgetItem(item[3].toString()));
    }
}

void Admin::populateParentCategoryComboBox()
{
    if (!parentCategoryComboBox)
        return;

    parentCategoryComboBox->clear();
    parentCategoryComboBox->addItem("Select Category", -1);

    const QList<QVariantList> menuItems = DatabaseUtils::fetchJewelryMenuItems();
    for (const auto &item : menuItems)
    {
        if (item.size() < 3)
            continue; // safety

        const int parentId = item[1].toInt();
        if (parentId == -1)  // top-level only
        {
            const QString categoryName = item[2].toString();
            const int categoryId = item[0].toInt();
            parentCategoryComboBox->addItem(categoryName, categoryId);
        }
    }
}

void Admin::on_add_menu_category_clicked()
{
    QString categoryName = categoryNameLineEdit->text().trimmed();
    if (categoryName.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "Category name cannot be empty.");
        return;
    }

    QString displayText = categoryName; // Categories use name as display text
    if (DatabaseUtils::insertJewelryMenuItem(-1, categoryName, displayText))
    {
        QMessageBox::information(this, "Success", "Category added successfully!");
        categoryNameLineEdit->clear();

        // ✅ Refresh both table and combo box
        populateJewelryMenuTable();
        populateParentCategoryComboBox();
    }
    else
    {
        QMessageBox::critical(this, "Error", "Failed to add category.");
    }
}

void Admin::on_add_menu_item_clicked()
{
    QString itemName = itemNameLineEdit->text().trimmed();
    QString displayText = displayTextLineEdit->text().trimmed();
    int parentId = parentCategoryComboBox->currentData().toInt();

    if (itemName.isEmpty() || displayText.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "Item name and display text cannot be empty.");
        return;
    }

    if (parentId == -1)
    {
        QMessageBox::warning(this, "Input Error", "Please select a parent category.");
        return;
    }

    if (DatabaseUtils::insertJewelryMenuItem(parentId, itemName, displayText))
    {
        QMessageBox::information(this, "Success", "Menu item added successfully!");

        //  Clear inputs
        itemNameLineEdit->clear();
        displayTextLineEdit->clear();
        parentCategoryComboBox->setCurrentIndex(0);

        //Refresh UI
        populateJewelryMenuTable();
        populateParentCategoryComboBox(); // keep combo in sync too
    }
    else
    {
        QMessageBox::critical(this, "Error", "Failed to add menu item.");
    }
}

void Admin::on_delete_menu_item_clicked()
{
    int row = jewelryMenuTable->currentRow();
    if (row < 0)
    {
        QMessageBox::warning(this, "No Selection", "Please select a menu item to delete.");
        return;
    }

    int id = jewelryMenuTable->item(row, 0)->text().toInt();
    QString name = jewelryMenuTable->item(row, 2)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Delete",
        QString("Are you sure you want to delete '%1'? This will also delete all sub-items.").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        if (DatabaseUtils::deleteJewelryMenuItem(id))
        {
            QMessageBox::information(this, "Success", "Menu item deleted successfully!");

            //Refresh both table and combo box
            populateJewelryMenuTable();
            populateParentCategoryComboBox();
        }
        else
        {
            QMessageBox::critical(this, "Error", "Failed to delete menu item.");
        }
    }
}

void Admin::on_admin_menu_push_button_clicked()
{
    if (!newAdminMenuButtons)
    {
        newAdminMenuButtons = new AdminMenuButtons(this);
        newAdminMenuButtons->setWindowFlags(Qt::FramelessWindowHint);
        newAdminMenuButtons->setAttribute(Qt::WA_DeleteOnClose, false);
        newAdminMenuButtons->setFixedSize(200, 350);

        // Position under the button
        QPoint localPos = ui->admin_menu_push_button->mapTo(this, QPoint(0, ui->admin_menu_push_button->height()));
        newAdminMenuButtons->move(localPos);

        connect(newAdminMenuButtons, &AdminMenuButtons::menuHidden, this, [=, this]()
                {
                    menuVisible = false;
                    qApp->removeEventFilter(this); // Always clean filter
                });

        // Connect signals
        connect(newAdminMenuButtons, &AdminMenuButtons::showImagesClicked, this, &Admin::on_show_images_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::updatePriceClicked, this, &Admin::on_update_price_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::addDiamondClicked, this, &Admin::on_add_dia_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::showUsersClicked, this, &Admin::on_show_users_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::jewelryMenuClicked, this, &Admin::on_jewelry_menu_button_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::logoutClicked, this, &Admin::on_logout_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::orderBookUsersPushButtonClicked, this, &Admin::on_orderBookUsersPushButton_clicked);
        connect(newAdminMenuButtons, &AdminMenuButtons::orderBookRequestPushButtonClicked, this, &Admin::on_orderBookRequestPushButton_clicked);
    }

    if (!menuVisible)
    {
        QPoint localPos = ui->admin_menu_push_button->mapTo(this, QPoint(0, ui->admin_menu_push_button->height()));
        newAdminMenuButtons->move(localPos);
        newAdminMenuButtons->show();
        newAdminMenuButtons->raise();
        qApp->installEventFilter(this);
        menuVisible = true;
    }
    else
    {
        newAdminMenuButtons->hide();
        qApp->removeEventFilter(this);
        menuVisible = false;
    }
}

void Admin::resizeEvent(QResizeEvent *event)
{
    if (menuVisible && newAdminMenuButtons)
    {
        QPoint localPos = ui->admin_menu_push_button->mapTo(this, QPoint(0, ui->admin_menu_push_button->height()));
        newAdminMenuButtons->move(localPos);
    }
    QDialog::resizeEvent(event);
}

void Admin::moveEvent(QMoveEvent *event)
{
    if (menuVisible && newAdminMenuButtons)
    {
        QPoint localPos = ui->admin_menu_push_button->mapTo(this, QPoint(0, ui->admin_menu_push_button->height()));
        newAdminMenuButtons->move(localPos);
    }
    QDialog::moveEvent(event);
}

bool Admin::eventFilter(QObject *obj, QEvent *event)
{
    if (menuVisible && newAdminMenuButtons && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QWidget *clickedWidget = QApplication::widgetAt(mouseEvent->globalPosition().toPoint());

        if (clickedWidget && !newAdminMenuButtons->isAncestorOf(clickedWidget) &&
            clickedWidget != newAdminMenuButtons)
        {
            newAdminMenuButtons->hide();
            menuVisible = false;
            qApp->removeEventFilter(this);
        }
    }

    return QDialog::eventFilter(obj, event);
}

void Admin::on_backToPageOnePushButton_clicked()
{
    ui->Admin_panel->setCurrentIndex(0);
}

void Admin::on_saveOrderBookPushButton_clicked()
{
    QString userId = ui->userIdLineEdit->text().trimmed();
    QString userName = ui->userNameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    QString confirmPassword = ui->confirmPasswordLineEdit->text();
    QString role = ui->roleComboBox->currentText();
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    // 1. Validation
    if (userId.isEmpty() || userName.isEmpty() || password.isEmpty() ||
        confirmPassword.isEmpty() || role.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "All fields are required.");
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "Password Error", "Passwords do not match.");
        return;
    }

    // 2. Call DatabaseUtils
    QString errorMsg;
    if (!DatabaseUtils::createOrderBookUser(userId, userName, password, role, date, errorMsg)) {
        QMessageBox::critical(this, "Database Error", errorMsg);
        return;
    }

    QMessageBox::information(this, "Success", "User account created successfully!");
    ui->Admin_panel->setCurrentIndex(0);

    // 3. Clear form
    ui->userIdLineEdit->clear();
    ui->userNameLineEdit->clear();
    ui->passwordLineEdit->clear();
    ui->confirmPasswordLineEdit->clear();
    ui->roleComboBox->setCurrentIndex(0);
}

void Admin::on_show_passwd_checkBox_toggled(bool checked)
{
    ui->admin_password_lineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}
