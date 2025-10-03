#include "jobsheet.h"
#include "ui_jobsheet.h"

#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QInputDialog>

#include "databaseutils.h"

JobSheet::JobSheet(QWidget *parent, const QString &jobNo, const QString &role)
    : QDialog(parent),
    ui(new Ui::JobSheet),
    userRole(role)
{
    ui->setupUi(this);

    setWindowTitle("Job Sheet");
    setMinimumSize(100, 100);
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    ui->gridLayout->setContentsMargins(4,4,4,4);

    int maxWidth = 1410;
    int maxHeight = 910;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int screenWidth = screenGeometry.width() * 0.8;
    int screenHeight = screenGeometry.height() * 0.8;

    // Choose the smaller between 80% of 1410×910 vs 80% of screen
    finalWidth = qMin(maxWidth, screenWidth);
    finalHeight = qMin(maxHeight, screenHeight);

    resize(finalWidth, finalHeight);
    move(screenGeometry.center() - rect().center());

    // Resize rows to fit contents
    ui->diamondAndStoneDetailTableWidget->resizeRowsToContents();
    ui->goldDetailTableWidget->resizeRowsToContents();

    // Function to adjust table height based on rows
    auto adjustTableHeight = [](QTableWidget* table){
        int totalHeight = table->horizontalHeader()->height();
        qDebug() << table->rowCount();
        for (int row = 0; row < table->rowCount(); ++row) {
            totalHeight += table->rowHeight(row);
        }
        totalHeight += 2 * table->frameWidth();
        table->setMinimumHeight(totalHeight);
        table->setMaximumHeight(totalHeight);
    };

    // Apply to both tables
    adjustTableHeight(ui->diamondAndStoneDetailTableWidget);
    adjustTableHeight(ui->goldDetailTableWidget);


    set_value(jobNo);
//     ui->extraNoteTextEdit->setText(R"(ACKNOWLEDGMENT OF ENTRUSTMENT
// We Hereby acknowledge receipt of the following goods mentioned overleaf which you have entrusted to melus and to which IAve hold in trust for you for the following purpose and on following conditions.
// (1) The goods have been entrusted to me/us for the sale purpose of being shown to intending purchasers of inspection.
// (2) The Goods remain your property and Iwe acquire no right to property or interest in them till sale note signed by you is passed or till the price is paid in respect there of not with standing the fact that mention is made of the rate or price in the particulars of goods herein behind set.
// (3) IWe agree not to sell or pledge, or montage or hypothecate the said goods or otherwise dea; with them in any manner till a sale note signed by you is passed or the price is paid to you. (4) The goods are to be returned to you forthwith whenever demanded back.
// (5) The Goods will be at my/out risk in all respect till a sale note signed by you is passed in respecte there of or ti the price is paid to you or till the goods are returned to you and  Awe am/are responsible to you for the retum of the said goods in the same condition as I/we have received the same.
// (6) Subject to Surat Jurisdiction.)");

//     ui->extraNoteTextEdit->setStyleSheet("font-size: 7.3pt;");
    // qDebug() << userRole;
    if (userRole == "designer") {
        set_value_designer();
        connect(ui->desigNoLineEdit, &QLineEdit::returnPressed, this, &JobSheet::loadImageForDesignNo);
    }
    else if(userRole == "manufacturer"){
        set_value_manuf();
    }
    else if(userRole == "manager"){
        set_value_manuf();
    }
}

JobSheet::~JobSheet()
{
    delete ui;
}

void JobSheet::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);  // Call base class

    if (!originalPixmap.isNull()) {
        ui->productImageLabel->setPixmap(
            originalPixmap.scaled(ui->productImageLabel->size(),
                                  Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation)
            );
    }

    int windowW = width();
    int windowH = height();

    int baseW = finalWidth;   // Store 1410 * 0.8 or whatever you use
    int baseH = finalHeight;

    int marginLeftRight = 4;
    int marginTopBottom = 4;

    // If window is larger than base, add extra margin to center content
    if (windowW > baseW)
        marginLeftRight = (windowW - baseW) / 2;

    if (windowH > baseH)
        marginTopBottom = (windowH - baseH) / 2;

    // Apply new margins
    ui->gridLayout->setContentsMargins(marginLeftRight, marginTopBottom,
                                       marginLeftRight, marginTopBottom);
}

void JobSheet::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if(ui->diamondAndStoneDetailTableWidget->hasFocus()){
            addTableRow(ui->diamondAndStoneDetailTableWidget);
        }
    }
    else
    {
        QDialog::keyPressEvent(event);
    }
}

void JobSheet::addTableRow(QTableWidget *table)
{
    int newRow = table->rowCount();
    table->insertRow(newRow);
}

void JobSheet::set_value(const QString &jobNo)
{
    auto dataOpt = DatabaseUtils::fetchJobSheetData(jobNo);
    if (!dataOpt) {
        qDebug() << "No record found for jobNo:" << jobNo;
        return;
    }
    const auto &data = *dataOpt;

    // Fill UI
    ui->jobIssuLineEdit->setText(data.sellerId);
    ui->orderPartyLineEdit->setText(data.partyId);
    ui->jobNoLineEdit->setText(data.jobNo);
    ui->orderNoLineEdit->setText(data.orderNo);
    ui->clientIdLineEdit->setText(data.clientId);

    ui->dateOrderDateEdit->setDate(QDate::fromString(data.orderDate, "yyyy-MM-dd"));
    ui->deliDateDateEdit->setDate(QDate::fromString(data.deliveryDate, "yyyy-MM-dd"));

    ui->itemDesignLineEdit->setText(QString::number(data.productPis));
    ui->desigNoLineEdit->setText(data.designNo);
    ui->purityLineEdit->setText(data.metalPurity);
    ui->metColLineEdit->setText(data.metalColor);

    ui->sizeNoLineEdit->setText(QString::number(data.sizeNo));
    ui->MMLineEdit->setText(QString::number(data.sizeMM));
    ui->lengthLineEdit->setText(QString::number(data.length));
    ui->widthLineEdit->setText(QString::number(data.width));
    ui->heightLineEdit->setText(QString::number(data.height));

    // Image
    if (!data.imagePath.isEmpty()) {
        QString fullPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/" + data.imagePath);
        originalPixmap.load(fullPath);

        // Optional: support high-DPI
        originalPixmap.setDevicePixelRatio(devicePixelRatioF());

        // Scale and set pixmap manually
        ui->productImageLabel->setPixmap(
            originalPixmap.scaled(ui->productImageLabel->size(),
                                  Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation)
            );
    }

    // Diamond & Stone
    auto [diamondJson, stoneJson] = DatabaseUtils::fetchDiamondAndStoneJson(data.designNo);

    QTableWidget *table = ui->diaAndStoneForDesignTableWidget;
    table->setRowCount(0);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Type", "Name", "Quantity", "Size (MM)"});

    auto parseAndAddRows = [&](const QString &jsonStr, const QString &typeLabel) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray())
            return;
        for (auto value : doc.array()) {
            if (!value.isObject()) continue;
            QJsonObject obj = value.toObject();
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(typeLabel));
            table->setItem(row, 1, new QTableWidgetItem(obj["type"].toString()));
            table->setItem(row, 2, new QTableWidgetItem(obj["quantity"].toString()));
            table->setItem(row, 3, new QTableWidgetItem(obj["sizeMM"].toString()));
        }
    };

    parseAndAddRows(diamondJson, "Diamond");
    parseAndAddRows(stoneJson, "Stone");
    table->resizeColumnsToContents();
}

void JobSheet::loadImageForDesignNo()
{
    QString designNo = ui->desigNoLineEdit->text().trimmed();
    if (designNo.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Design number is empty.");
        return;
    }

    // --- Fetch image path from DB ---
    QString imagePath = DatabaseUtils::fetchImagePathForDesign(designNo);
    if (imagePath.isEmpty()) {
        QMessageBox::information(this, "Not Found", "No image found for design number: " + designNo);
        return;
    }

    QString fullPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/" + imagePath);
    QPixmap pixmap(fullPath);

    if (!pixmap.isNull()) {
        ui->productImageLabel->setPixmap(
            pixmap.scaled(ui->productImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)
            );
        // ✅ Save into OrderBook-Detail
        saveDesignNoAndImagePath(designNo, imagePath);
    } else {
        QMessageBox::warning(this, "Image Error", "Image not found at path:\n" + fullPath);
        return;
    }

    // --- Fill diamond & stone table ---
    DatabaseUtils::fillStoneTable(ui->diaAndStoneForDesignTableWidget, designNo);
}

void JobSheet::saveDesignNoAndImagePath(const QString &designNo, const QString &imagePath)
{
    QString jobNo = ui->jobNoLineEdit->text().trimmed();
    if (jobNo.isEmpty()) {
        QMessageBox::warning(this, "Missing Data", "Job No is missing. Cannot save image path.");
        return;
    }

    if (!DatabaseUtils::updateDesignNoAndImagePath(jobNo, designNo, imagePath)) {
        QMessageBox::critical(this, "Query Error", "Failed to update OrderBook-Detail.");
    }
}

void JobSheet::set_value_designer(){
    QList<QLineEdit*> lineEdits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        if (edit != ui->desigNoLineEdit) {
            edit->setReadOnly(true);
        } else if (userRole != "designer") {
            edit->setReadOnly(true);  // disable even this for non-designers
        }
    }

    QList<QDateEdit*> dateEdits = findChildren<QDateEdit*>();
    for (QDateEdit* dateEdit : dateEdits) {
        dateEdit->setEnabled(false);
    }

    QList<QTextEdit*> textEdits = findChildren<QTextEdit*>();
    for (QTextEdit* textEdit : textEdits) {
        textEdit->setReadOnly(true);
    }

    QList<QComboBox*> comboBoxes = findChildren<QComboBox*>();
    for (QComboBox* combo : comboBoxes) {
        combo->setEnabled(false);
    }

    QList<QTableWidget*> tableWidgets = findChildren<QTableWidget*>();
    for (QTableWidget* table : tableWidgets) {
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setEnabled(false);
    }

    QList<QLabel*> labels = findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label != ui->productImageLabel) {
            label->setEnabled(false);
        } else if (userRole != "designer" && label != ui->productImageLabel) {
            label->setEnabled(false); // restrict image even for label if not designer
        }
    }

}

void JobSheet::set_value_manuf()
{
    // Selection behavior
    ui->goldDetailTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->goldDetailTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);

    // ✅ Fill totals when opening JobSheet
    updateGoldTotalWeight();

    // ✅ Open ManageGold (col 1) or manageGoldReturn (col 4) on click
    connect(ui->goldDetailTableWidget, &QTableWidget::cellClicked, this, [this](int row, int col) {
        if (row == 0 && (col == 1 || col == 4)) {
            QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                ui->goldDetailTableWidget->setItem(row, col, item);
            }
            onGoldDetailCellClicked(item);
        }
        // ✅ Special case: Dust cell (0,2) → open QInputDialog
        else if (row == 0 && col == 2) {
            handleCellSave(row, col); // this will trigger the QInputDialog version
        }
    });

    // ✅ Allow manual editing ONLY for returns (col 4, row 1–4)
    ui->goldDetailTableWidget->setEditTriggers(
        QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
        );

    connect(ui->goldDetailTableWidget, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (col == 4 && row >= 1 && row <= 4) { // returns
            handleCellSave(row, col);
        }
    });

    // ✅ Lock col 1, col 2 (dust is read-only in table), col 3, and col 5
    for (int r = 0; r < ui->goldDetailTableWidget->rowCount(); ++r) {
        for (int c : {1, 2, 3, 5}) {
            QTableWidgetItem *item = ui->goldDetailTableWidget->item(r, c);
            if (!item) {
                item = new QTableWidgetItem();
                ui->goldDetailTableWidget->setItem(r, c, item);
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); // make read-only
        }
    }
}

void JobSheet::onGoldDetailCellClicked(QTableWidgetItem *item)
{
    int row = item->row();
    int col = item->column();

    // =========================
    // Filling Gold (col = 1)
    // =========================
    if (row == 0 && col == 1) {
        if (!newManageGold) {
            newManageGold = new ManageGold(this);
            newManageGold->setWindowFlags(Qt::FramelessWindowHint);
            newManageGold->setAttribute(Qt::WA_DeleteOnClose, false);

            // Position under the cell
            QRect cellRect = ui->goldDetailTableWidget->visualItemRect(item);
            QPoint localPos = ui->goldDetailTableWidget->mapTo(this, cellRect.bottomLeft());
            newManageGold->move(localPos);

            connect(newManageGold, &ManageGold::menuHidden, this, [=, this]() {
                manageGold = false;
                qApp->removeEventFilter(this);
            });

            connect(newManageGold, &ManageGold::totalWeightCalculated, this, [=](double weight) {
                int r = 0, c = 1;
                QTableWidgetItem *cell = ui->goldDetailTableWidget->item(r, c);
                if (!cell) {
                    cell = new QTableWidgetItem();
                    ui->goldDetailTableWidget->setItem(r, c, cell);
                }
                cell->setText(QString::number(weight, 'f', 3));
            });
        }

        if (!manageGold) {
            QRect cellRect = ui->goldDetailTableWidget->visualItemRect(item);
            QPoint localPos = ui->goldDetailTableWidget->mapTo(this, cellRect.bottomLeft());
            newManageGold->move(localPos);
            newManageGold->show();
            newManageGold->raise();
            qApp->installEventFilter(this);
            manageGold = true;
        } else {
            newManageGold->hide();
            qApp->removeEventFilter(this);
            manageGold = false;
        }
    }

    // =========================
    // Returning Gold (col = 4)
    // =========================
    if (row == 0 && col == 4) {
        if (!newManageGoldReturn) {
            newManageGoldReturn = new ManageGoldReturn(this);  // ⚡ reuse ManageGold or make manageGoldReturn class
            newManageGoldReturn->setWindowFlags(Qt::FramelessWindowHint);
            newManageGoldReturn->setAttribute(Qt::WA_DeleteOnClose, false);

            // Position under the cell
            QRect cellRect = ui->goldDetailTableWidget->visualItemRect(item);
            QPoint localPos = ui->goldDetailTableWidget->mapTo(this, cellRect.bottomLeft());
            newManageGoldReturn->move(localPos);

            connect(newManageGoldReturn, &ManageGoldReturn::menuHidden, this, [=, this]() {
                manageGoldReturn = false;
                qApp->removeEventFilter(this);
            });

            connect(newManageGoldReturn, &ManageGoldReturn::totalWeightCalculated, this, [=](double weight) {
                int r = 0, c = 4;
                QTableWidgetItem *cell = ui->goldDetailTableWidget->item(r, c);
                if (!cell) {
                    cell = new QTableWidgetItem();
                    ui->goldDetailTableWidget->setItem(r, c, cell);
                }
                cell->setText(QString::number(weight, 'f', 3));
            });
        }

        if (!manageGoldReturn) {
            QRect cellRect = ui->goldDetailTableWidget->visualItemRect(item);
            QPoint localPos = ui->goldDetailTableWidget->mapTo(this, cellRect.bottomLeft());
            newManageGoldReturn->move(localPos);
            newManageGoldReturn->show();
            newManageGoldReturn->raise();
            qApp->installEventFilter(this);
            manageGoldReturn = true;
        } else {
            newManageGoldReturn->hide();
            qApp->removeEventFilter(this);
            manageGoldReturn = false;
        }
    }
    updateGoldTotalWeight();
}

void JobSheet::updateGoldTotalWeight()
{
    QString jobNo = ui->jobNoLineEdit->text().trimmed();
    if (jobNo.isEmpty())
        return;

    double totalIssueWeight = 0.0;
    double totalReturnWeight = 0.0;
    double dustWeight = 0.0;

    double buffingReturn = 0.0;
    double freePolishReturn = 0.0;
    double settingReturn = 0.0;
    double finalPolishReturn = 0.0;

    // Open DB
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "weight_fetch_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    QString returnJson; // save return JSON for product weight

    if (db.open()) {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT filling_issue, filling_dust, filling_return,
                   buffing_return, free_polish_return, setting_return, final_polish_return
            FROM jobsheet_detail WHERE job_no = ?
        )");
        query.addBindValue(jobNo);

        if (query.exec() && query.next()) {
            // ---------------- Issue JSON ----------------
            QString issueJson = query.value(0).toString();
            if (!issueJson.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(issueJson.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                    QJsonArray arr = doc.array();
                    for (const QJsonValue &val : arr) {
                        if (val.isObject()) {
                            QJsonObject obj = val.toObject();
                            totalIssueWeight += obj["weight"].toString().toDouble();
                        }
                    }
                }
            }

            // ---------------- Dust (plain value) ----------------
            QString dustVal = query.value(1).toString();
            if (!dustVal.isEmpty()) {
                dustWeight = dustVal.toDouble();
                int row = 0, col = 2;
                QTableWidgetItem *dustItem = ui->goldDetailTableWidget->item(row, col);
                if (!dustItem) {
                    dustItem = new QTableWidgetItem();
                    ui->goldDetailTableWidget->setItem(row, col, dustItem);
                }
                dustItem->setText(QString::number(dustWeight, 'f', 3));
            }

            // ---------------- Return JSON ----------------
            returnJson = query.value(2).toString();
            if (!returnJson.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(returnJson.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                    QJsonArray arr = doc.array();
                    for (const QJsonValue &val : arr) {
                        if (val.isObject()) {
                            QJsonObject obj = val.toObject();
                            totalReturnWeight += obj["weight"].toString().toDouble();
                        }
                    }
                }
            }

            // ---------------- Stage Returns ----------------
            buffingReturn     = query.value(3).toDouble();
            freePolishReturn  = query.value(4).toDouble();
            settingReturn     = query.value(5).toDouble();
            finalPolishReturn = query.value(6).toDouble();

            // Show in col=4
            auto setCell = [this](int row, int col, double val) {
                QTableWidgetItem *it = ui->goldDetailTableWidget->item(row, col);
                if (!it) {
                    it = new QTableWidgetItem();
                    ui->goldDetailTableWidget->setItem(row, col, it);
                }
                if (val != 0.0)
                    it->setText(QString::number(val, 'f', 3));
            };
            setCell(1, 4, buffingReturn);
            setCell(2, 4, freePolishReturn);
            setCell(3, 4, settingReturn);
            setCell(4, 4, finalPolishReturn);
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("weight_fetch_conn");

    // ---------------- Helper for product weight ----------------
    auto getProductWeight = [](const QString &returnJson) -> double {
        if (returnJson.isEmpty()) return 0.0;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(returnJson.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) return 0.0;

        double productWeight = 0.0;
        QJsonArray arr = doc.array();
        for (const QJsonValue &val : arr) {
            if (val.isObject()) {
                QJsonObject obj = val.toObject();
                if (obj.contains("type") && obj["type"].toString() == "Product") {
                    productWeight += obj["weight"].toString().toDouble();
                }
            }
        }
        return productWeight;
    };

    // ✅ Issue total → col 1 (row 0)
    {
        int row = 0, col = 1;
        QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, col, item);
        }
        item->setText(QString::number(totalIssueWeight, 'f', 3));
    }

    // ✅ Return total (all returns combined) → col 4 (row 0)
    {
        double grandReturn = totalReturnWeight;
        int row = 0, col = 4;
        QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, col, item);
        }
        item->setText(QString::number(grandReturn, 'f', 3));
    }

    // ✅ Stage col 1
    // (1,1) → product weight only
    {
        double productWeight = getProductWeight(returnJson);
        QTableWidgetItem *item11 = ui->goldDetailTableWidget->item(1, 1);
        if (!item11) {
            item11 = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(1, 1, item11);
        }
        if (productWeight != 0.0)
            item11->setText(QString::number(productWeight, 'f', 3));
    }

    // Copy returns into stage col 1
    auto copyCell = [this](int fromRow, int fromCol, int toRow, int toCol) {
        QTableWidgetItem *src = ui->goldDetailTableWidget->item(fromRow, fromCol);
        if (!src || src->text().isEmpty()) return;
        QTableWidgetItem *dst = ui->goldDetailTableWidget->item(toRow, toCol);
        if (!dst) {
            dst = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(toRow, toCol, dst);
        }
        dst->setText(src->text());
    };
    copyCell(1, 4, 2, 1);
    copyCell(2, 4, 3, 1);
    copyCell(3, 4, 4, 1);

    // ---------------- Loss & Return% for stage rows ----------------
    auto calcLossReturn = [this](int row) {
        QTableWidgetItem *inputItem = ui->goldDetailTableWidget->item(row, 1);
        QTableWidgetItem *returnItem = ui->goldDetailTableWidget->item(row, 4);
        if (!inputItem || !returnItem) return;

        double input = inputItem->text().toDouble();
        double ret = returnItem->text().toDouble();

        double loss = input - ret;
        double retPercent = (input > 0.0) ? (ret / input) * 100.0 : 0.0;

        QTableWidgetItem *lossItem = ui->goldDetailTableWidget->item(row, 3);
        if (!lossItem) {
            lossItem = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, 3, lossItem);
        }
        lossItem->setText(QString::number(loss, 'f', 3));

        QTableWidgetItem *percentItem = ui->goldDetailTableWidget->item(row, 5);
        if (!percentItem) {
            percentItem = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, 5, percentItem);
        }
        percentItem->setText(QString::number(retPercent, 'f', 2) + "%");
    };

    calcLossReturn(1);
    calcLossReturn(2);
    calcLossReturn(3);
    calcLossReturn(4);

    // ✅ Loss & Return% for row 0 (with dust)
    {
        double grandReturn = totalReturnWeight;
        double loss = totalIssueWeight - (grandReturn + dustWeight);
        int row = 0, col = 3;
        QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, col, item);
        }
        item->setText(QString::number(loss, 'f', 3));

        double returnPercent = (totalIssueWeight > 0.0) ? ((grandReturn + dustWeight) / totalIssueWeight) * 100.0 : 0.0;
        row = 0; col = 5;
        item = ui->goldDetailTableWidget->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, col, item);
        }
        item->setText(QString::number(returnPercent, 'f', 2) + "%");
    }

    // ---------------- Total row (row 5) ----------------
    // ---------------- Total row (row 5) ----------------
    {
        double lossSum = 0.0;

        for (int row = 0; row <= 4; ++row) {
            // Sum of Loss (col 3)
            QTableWidgetItem *lossItem = ui->goldDetailTableWidget->item(row, 3);
            if (lossItem && !lossItem->text().isEmpty())
                lossSum += lossItem->text().toDouble();
        }

        // Set total Loss in (5,3)
        QTableWidgetItem *totalLossItem = ui->goldDetailTableWidget->item(5, 3);
        if (!totalLossItem) {
            totalLossItem = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(5, 3, totalLossItem);
        }
        totalLossItem->setText(QString::number(lossSum, 'f', 3));

        // Calculate total return%: (0,1 - 5,3) / 0,1 * 100
        QTableWidgetItem *issueItem = ui->goldDetailTableWidget->item(0, 1);
        double totalIssue = issueItem ? issueItem->text().toDouble() : 0.0;
        double totalReturnPercent = 0.0;
        if (totalIssue > 0.0) {
            totalReturnPercent = ((totalIssue - lossSum) / totalIssue) * 100.0;
        }

        // Set total return% in (5,5)
        QTableWidgetItem *totalReturnItem = ui->goldDetailTableWidget->item(5, 5);
        if (!totalReturnItem) {
            totalReturnItem = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(5, 5, totalReturnItem);
        }
        totalReturnItem->setText(QString::number(totalReturnPercent, 'f', 2) + "%");
    }

}

void JobSheet::handleCellSave(int row, int col)
{
    QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
    if (!item) return;

    QString jobNo = ui->jobNoLineEdit->text().trimmed();
    if (jobNo.isEmpty()) return;

    // Map row/col to database column name
    QString dbColumn;
    if (row == 0 && col == 2) dbColumn = "filling_dust";       // Dust (special case)
    else if (row == 1 && col == 4) dbColumn = "buffing_return";
    else if (row == 2 && col == 4) dbColumn = "free_polish_return";
    else if (row == 3 && col == 4) dbColumn = "setting_return";
    else if (row == 4 && col == 4) dbColumn = "final_polish_return";
    else return; // not a handled cell

    // Open DB
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cell_save_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    double existingValue = 0.0;
    if (db.open()) {
        QSqlQuery q(db);
        q.prepare("SELECT " + dbColumn + " FROM jobsheet_detail WHERE job_no = ?");
        q.addBindValue(jobNo);
        if (q.exec() && q.next()) {
            existingValue = q.value(0).toDouble();
        }
    }

    // ✅ Special case: Dust (row 0, col 2) → always editable, ADD instead of overwrite
    if (row == 0 && col == 2) {
        bool okInput = false;
        double addValue = QInputDialog::getDouble(
            this,
            "Add Dust Weight",
            "Enter additional dust weight:",
            0.0,        // default
            0.0,        // min
            100000.0,   // max
            3,          // decimals
            &okInput
            );

        if (!okInput) {
            // user cancelled → restore old value
            item->setText(QString::number(existingValue, 'f', 3));
            db.close();
            QSqlDatabase::removeDatabase("cell_save_conn");
            return;
        }

        double newTotal = existingValue + addValue;
        QString newFormatted = QString::number(newTotal, 'f', 3);

        QSqlQuery q(db);
        q.prepare("UPDATE jobsheet_detail SET " + dbColumn + " = ? WHERE job_no = ?");
        q.addBindValue(newFormatted);
        q.addBindValue(jobNo);
        if (!q.exec() || q.numRowsAffected() == 0) {
            q.prepare("INSERT INTO jobsheet_detail (job_no, " + dbColumn + ") VALUES (?, ?)");
            q.addBindValue(jobNo);
            q.addBindValue(newFormatted);
            q.exec();
        }

        item->setText(newFormatted); // show updated total
        db.close();
        QSqlDatabase::removeDatabase("cell_save_conn");
        updateGoldTotalWeight();
        return;
    }

    // ✅ For other cells (one-time save)
    QString text = item->text().trimmed();
    if (text.isEmpty()) return;

    bool ok;
    double value = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Invalid", "Please enter a valid number.");
        item->setText(""); // reset
        return;
    }

    QString formatted = QString::number(value, 'f', 3);

    bool alreadyExists = false;
    QString existingText;
    {
        QSqlQuery q(db);
        q.prepare("SELECT " + dbColumn + " FROM jobsheet_detail WHERE job_no = ?");
        q.addBindValue(jobNo);
        if (q.exec() && q.next()) {
            existingText = q.value(0).toString();
            if (!existingText.isEmpty()) alreadyExists = true;
        }
    }

    if (alreadyExists) {
        QMessageBox::warning(this, "Locked", "This cell is already filled and cannot be changed.");
        item->setText(existingText);
        db.close();
        QSqlDatabase::removeDatabase("cell_save_conn");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirm", "Save value " + formatted + " ?");
    if (reply == QMessageBox::Yes) {
        QSqlQuery q(db);
        q.prepare("UPDATE jobsheet_detail SET " + dbColumn + " = ? WHERE job_no = ?");
        q.addBindValue(formatted);
        q.addBindValue(jobNo);
        if (!q.exec() || q.numRowsAffected() == 0) {
            q.prepare("INSERT INTO jobsheet_detail (job_no, " + dbColumn + ") VALUES (?, ?)");
            q.addBindValue(jobNo);
            q.addBindValue(formatted);
            q.exec();
        }
        item->setText(formatted);
    } else {
        item->setText("");
    }

    db.close();
    QSqlDatabase::removeDatabase("cell_save_conn");
    updateGoldTotalWeight();
}
