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

    ui->gridLayout->setContentsMargins(10,10,10,10);

    // Your base design resolution
    // --- Scaling setup ---
    // Base resolution you designed on
    const double baseW = 1920.0;
    const double baseH = 1080.0;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // Compute scale factor relative to base resolution
    double scaleX = screenGeometry.width() / baseW;
    double scaleY = screenGeometry.height() / baseH;
    scaleFactor = qMin(scaleX, scaleY);

    // Scale main window size
    finalWidth  = static_cast<int>(1410 * scaleFactor);
    finalHeight = static_cast<int>(910  * scaleFactor);

    resize(finalWidth, finalHeight);
    move(screenGeometry.center() - rect().center());

    // Apply global font scaling
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * scaleFactor);
    setFont(f);

    // Apply scaled margins
    ui->gridLayout->setContentsMargins(
        static_cast<int>(4 * scaleFactor),
        static_cast<int>(4 * scaleFactor),
        static_cast<int>(4 * scaleFactor),
        static_cast<int>(4 * scaleFactor));



    resize(finalWidth, finalHeight);
    move(screenGeometry.center() - rect().center());

    // Resize rows to fit contents
    ui->diamondAndStoneDetailTableWidget->resizeRowsToContents();
    ui->goldDetailTableWidget->resizeRowsToContents();

    // Function to adjust table height based on rows
    // Scale table row heights and headers
    auto scaleTable = [this](QTableWidget* table) {
        table->horizontalHeader()->setDefaultSectionSize(
            static_cast<int>(table->horizontalHeader()->defaultSectionSize() * scaleFactor));
        table->verticalHeader()->setDefaultSectionSize(
            static_cast<int>(table->verticalHeader()->defaultSectionSize() * scaleFactor));
        table->resizeRowsToContents();
    };

    scaleTable(ui->diamondAndStoneDetailTableWidget);
    scaleTable(ui->goldDetailTableWidget);


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
    QDialog::resizeEvent(event);

    if (!originalPixmap.isNull()) {
        ui->productImageLabel->setPixmap(
            originalPixmap.scaled(
                ui->productImageLabel->size() * scaleFactor,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    }

    int marginLeftRight = static_cast<int>(4 * scaleFactor);
    int marginTopBottom = static_cast<int>(4 * scaleFactor);

    int windowW = width();
    int windowH = height();

    if (windowW > finalWidth)
        marginLeftRight = (windowW - finalWidth) / 2;
    if (windowH > finalHeight)
        marginTopBottom = (windowH - finalHeight) / 2;

    ui->gridLayout->setContentsMargins(
        marginLeftRight, marginTopBottom,
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
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"Type", "Name", "Quantity", "Size (MM)", "1 Pc Wt", "Total Wt"});

    double diamondTotalWt = 0.0;
    double stoneTotalWt   = 0.0;

    auto parseAndAddRows = [&](const QString &jsonStr, const QString &typeLabel) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray())
            return;

        for (auto value : doc.array()) {
            if (!value.isObject()) continue;
            QJsonObject obj = value.toObject();

            QString name  = obj["type"].toString();
            QString size  = obj["sizeMM"].toString();
            int qty       = obj["quantity"].toString().toInt();
            double wtEach = obj["weight"].toDouble();
            double wtTotal = wtEach * qty;

            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(typeLabel));
            table->setItem(row, 1, new QTableWidgetItem(name));
            table->setItem(row, 2, new QTableWidgetItem(QString::number(qty)));
            table->setItem(row, 3, new QTableWidgetItem(size));
            table->setItem(row, 4, new QTableWidgetItem(QString::number(wtEach, 'f', 3)));
            table->setItem(row, 5, new QTableWidgetItem(QString::number(wtTotal, 'f', 3)));

            // ✅ Accumulate totals
            if (typeLabel == "Diamond")
                diamondTotalWt += wtTotal;
            else if (typeLabel == "Stone")
                stoneTotalWt += wtTotal;
        }
    };

    parseAndAddRows(diamondJson, "Diamond");
    parseAndAddRows(stoneJson, "Stone");

    table->resizeColumnsToContents();

    // ✅ Update total line edits
    ui->diamondTotalLineEdit->setText(QString::number(diamondTotalWt, 'f', 3));
    ui->stoneTotalLineEdit->setText(QString::number(stoneTotalWt, 'f', 3));
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
        // Disable editing
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // Optional: disable selection if you don't want the user to highlight cells
        // table->setSelectionMode(QAbstractItemView::NoSelection);

        // Make it read-only but still scrollable and interactive
        table->setFocusPolicy(Qt::NoFocus);
        table->setStyleSheet("QTableWidget { background-color: #f0f0f0; }"); // optional visual cue
    }

    QList<QLabel*> labels = findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label != ui->productImageLabel) {
            label->setEnabled(false);
        } else if (userRole != "designer" && label != ui->productImageLabel) {
            label->setEnabled(false); // restrict image even for label if not designer
        }
    }

    updateDiamondTotals();
    updateGoldTotalWeight();
}

void JobSheet::set_value_manuf()
{
    //
    // ---------------------- GOLD TABLE ----------------------
    //
    ui->goldDetailTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->goldDetailTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);

    updateGoldTotalWeight();

    // ✅ Open ManageGold for col 1 (Filling), col 2 (Dust), col 4 (Return)
    connect(ui->goldDetailTableWidget, &QTableWidget::cellClicked, this, [this](int row, int col) {
        if (row == 0 && (col == 1 || col == 2 || col == 4)) {
            QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                ui->goldDetailTableWidget->setItem(row, col, item);
            }
            onGoldDetailCellClicked(item);
        }
    });

    ui->goldDetailTableWidget->setEditTriggers(
        QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
        );

    connect(ui->goldDetailTableWidget, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (col == 4 && row >= 1 && row <= 4) {
            handleCellSave(row, col);
        }
    });

    // ✅ Lock all except return (col 4 rows 1–4)
    for (int r = 0; r < ui->goldDetailTableWidget->rowCount(); ++r) {
        for (int c : {1, 2, 3, 5}) {
            QTableWidgetItem *item = ui->goldDetailTableWidget->item(r, c);
            if (!item) {
                item = new QTableWidgetItem();
                ui->goldDetailTableWidget->setItem(r, c, item);
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }

    //
    // ---------------------- DIAMOND & STONE TABLE ----------------------
    //
    ui->diamondAndStoneDetailTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->diamondAndStoneDetailTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);

    // ✅ Make every cell non-editable
    for (int r = 0; r < ui->diamondAndStoneDetailTableWidget->rowCount(); ++r) {
        for (int c = 0; c < ui->diamondAndStoneDetailTableWidget->columnCount(); ++c) {
            QTableWidgetItem *item = ui->diamondAndStoneDetailTableWidget->item(r, c);
            if (!item) {
                item = new QTableWidgetItem();
                ui->diamondAndStoneDetailTableWidget->setItem(r, c, item);
            }
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }

    setupDiamondIssueClicks();
    updateDiamondTotals();
    updateGoldTotalWeight();
    // ✅ Handle clicks to open DiamonIssueRetBro window


}

void JobSheet::onGoldDetailCellClicked(QTableWidgetItem *item)
{
    int row = item->row();
    int col = item->column();

    // =========================
    // Filling Gold (col = 1)
    // =========================
    if (row == 0 && (col == 1 || col == 2 || col == 4)) {
        if (!newManageGold) {
            newManageGold = new ManageGold(this);
            newManageGold->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
            newManageGold->setAttribute(Qt::WA_DeleteOnClose, false);

            connect(newManageGold, &ManageGold::menuHidden, this, [this]() {
                qApp->removeEventFilter(this);
                manageGold = false;
            });

            connect(newManageGold, &ManageGold::totalWeightCalculated, this, [this](double weight) {
                int r = 0;
                int c = (newManageGold->currentMode == ManageGold::Filling) ? 1 : 4;
                QTableWidgetItem *cell = ui->goldDetailTableWidget->item(r, c);
                if (!cell)
                    cell = new QTableWidgetItem();
                cell->setText(QString::number(weight, 'f', 3));
                ui->goldDetailTableWidget->setItem(r, c, cell);
            });
        }

        // ✅ Set mode dynamically
        newManageGold->setMode(
            col == 1 ? ManageGold::Filling :
                col == 4 ? ManageGold::Returning :
                ManageGold::Dust        // ✅ new mode for dust
            );


        QRect cellRect = ui->goldDetailTableWidget->visualItemRect(item);
        QPoint globalPos = ui->goldDetailTableWidget->viewport()->mapToGlobal(cellRect.bottomLeft());

        // ✅ Toggle behavior — hide if already visible
        if (newManageGold->isVisible()) {
            newManageGold->hide();
            manageGold = false;
            qApp->removeEventFilter(this);
        } else {
            newManageGold->move(globalPos);
            newManageGold->show();
            newManageGold->raise();
            qApp->installEventFilter(this);
            manageGold = true;
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

    // --- Open DB ---
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "weight_fetch_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    QString returnJson;

    if (db.open()) {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT filling_issue, filling_dust, filling_return,
                   buffing_return, free_polish_return, setting_return, final_polish_return
            FROM jobsheet_detail WHERE job_no = ?
        )");
        query.addBindValue(jobNo);

        if (query.exec() && query.next()) {
            // --- Issue JSON ---
            QString issueJson = query.value(0).toString();
            if (!issueJson.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(issueJson.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                    for (const QJsonValue &val : doc.array())
                        if (val.isObject())
                            totalIssueWeight += val.toObject()["weight"].toString().toDouble();
                }
            }

            // --- Dust JSON ---
            QString dustJson = query.value(1).toString();
            if (!dustJson.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(dustJson.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                    for (const QJsonValue &val : doc.array())
                        if (val.isObject())
                            dustWeight += val.toObject()["weight"].toString().toDouble();
                } else {
                    bool ok = false;
                    double plainDust = dustJson.toDouble(&ok);
                    if (ok) dustWeight = plainDust;
                }

                int row = 0, col = 2;
                QTableWidgetItem *dustItem = ui->goldDetailTableWidget->item(row, col);
                if (!dustItem) dustItem = new QTableWidgetItem();
                ui->goldDetailTableWidget->setItem(row, col, dustItem);
                dustItem->setText(QString::number(dustWeight, 'f', 3));
            }

            // --- Return JSON ---
            returnJson = query.value(2).toString();
            if (!returnJson.isEmpty()) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(returnJson.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                    for (const QJsonValue &val : doc.array())
                        if (val.isObject())
                            totalReturnWeight += val.toObject()["weight"].toString().toDouble();
                }
            }

            // --- Stage returns ---
            buffingReturn     = query.value(3).toDouble();
            freePolishReturn  = query.value(4).toDouble();
            settingReturn     = query.value(5).toDouble();
            finalPolishReturn = query.value(6).toDouble();

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

    // --- Helper: product weight ---
    auto getProductWeight = [](const QString &returnJson) -> double {
        if (returnJson.isEmpty()) return 0.0;
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(returnJson.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) return 0.0;
        double productWeight = 0.0;
        for (const QJsonValue &val : doc.array())
            if (val.isObject() && val.toObject()["type"].toString() == "Product")
                productWeight += val.toObject()["weight"].toString().toDouble();
        return productWeight;
    };

    // --- Fill base table values ---
    auto setItemVal = [this](int row, int col, double val) {
        QTableWidgetItem *it = ui->goldDetailTableWidget->item(row, col);
        if (!it) it = new QTableWidgetItem();
        ui->goldDetailTableWidget->setItem(row, col, it);
        it->setText(QString::number(val, 'f', 3));
    };
    setItemVal(0, 1, totalIssueWeight);
    setItemVal(0, 4, totalReturnWeight);

    double productWeight = getProductWeight(returnJson);
    setItemVal(1, 1, productWeight);

    auto copyCell = [this](int fromRow, int fromCol, int toRow, int toCol) {
        QTableWidgetItem *src = ui->goldDetailTableWidget->item(fromRow, fromCol);
        if (!src || src->text().isEmpty()) return;
        QTableWidgetItem *dst = ui->goldDetailTableWidget->item(toRow, toCol);
        if (!dst) dst = new QTableWidgetItem();
        ui->goldDetailTableWidget->setItem(toRow, toCol, dst);
        dst->setText(src->text());
    };
    copyCell(1, 4, 2, 1);
    copyCell(2, 4, 3, 1);
    copyCell(3, 4, 4, 1);

    // --- Calculate diamond/stone/other used weights ---
    auto getUsedWeight = [this](int row) -> double {
        QTableWidget *tbl = ui->diamondAndStoneDetailTableWidget;
        auto val = [&](int c) -> double {
            QTableWidgetItem *it = tbl->item(row, c);


            return (it && !it->text().isEmpty()) ? it->text().toDouble() : 0.0;
        };
        // qDebug() << "Row:" << row << "Cols:" << val(2) << val(4) << val(6);
        return val(2) - val(4) - val(6);
    };
    double diamondUsed = getUsedWeight(0);
    double stoneUsed   = getUsedWeight(1);
    double otherUsed   = getUsedWeight(2);
    double totalUsedDSO = diamondUsed + stoneUsed + otherUsed;
    qDebug() << totalUsedDSO;

    // --- Loss & Loss% calculations ---
    auto calcLossPercent = [this](int row, double adjustExtra = 0.0) {
        QTableWidgetItem *inputItem  = ui->goldDetailTableWidget->item(row, 1);
        QTableWidgetItem *returnItem = ui->goldDetailTableWidget->item(row, 4);
        if (!inputItem || !returnItem) return;

        double input = inputItem->text().toDouble();
        double ret   = returnItem->text().toDouble() - adjustExtra; // subtract extra weights
        double loss = input - ret;
        double lossPercent = (input > 0.0) ? (loss / input) * 100.0 : 0.0;

        auto setCellVal = [this](int row, int col, const QString &text) {
            QTableWidgetItem *it = ui->goldDetailTableWidget->item(row, col);
            if (!it) it = new QTableWidgetItem();
            ui->goldDetailTableWidget->setItem(row, col, it);
            it->setText(text);
        };
        setCellVal(row, 3, QString::number(loss, 'f', 3));
        setCellVal(row, 5, QString::number(lossPercent, 'f', 2) + "%");
    };

    calcLossPercent(1);
    calcLossPercent(2);
    calcLossPercent(3, totalUsedDSO); // subtract diamond + stone + other
    calcLossPercent(4);

    // --- Overall loss (row 0, including dust) ---
    {
        double grandReturn = totalReturnWeight;
        double loss = totalIssueWeight - (grandReturn + dustWeight);
        setItemVal(0, 3, loss);

        double lossPercent = (totalIssueWeight > 0.0)
                                 ? (loss / totalIssueWeight) * 100.0
                                 : 0.0;
        QTableWidgetItem *percentItem = ui->goldDetailTableWidget->item(0, 5);
        if (!percentItem) percentItem = new QTableWidgetItem();
        ui->goldDetailTableWidget->setItem(0, 5, percentItem);
        percentItem->setText(QString::number(lossPercent, 'f', 2) + "%");
    }

    // --- Total row (row 5) ---
    {
        double lossSum = 0.0;
        for (int row = 0; row <= 4; ++row) {
            QTableWidgetItem *lossItem = ui->goldDetailTableWidget->item(row, 3);
            if (lossItem && !lossItem->text().isEmpty())
                lossSum += lossItem->text().toDouble();
        }

        setItemVal(5, 3, lossSum);

        QTableWidgetItem *issueItem = ui->goldDetailTableWidget->item(0, 1);
        double totalIssue = issueItem ? issueItem->text().toDouble() : 0.0;
        double totalLossPercent = (totalIssue > 0.0)
                                      ? (lossSum / totalIssue) * 100.0
                                      : 0.0;

        QTableWidgetItem *totalLossItem = ui->goldDetailTableWidget->item(5, 5);
        if (!totalLossItem) totalLossItem = new QTableWidgetItem();
        ui->goldDetailTableWidget->setItem(5, 5, totalLossItem);
        totalLossItem->setText(QString::number(totalLossPercent, 'f', 2) + "%");
    }

    // ✅ Optional: rename header to "Loss %"
    ui->goldDetailTableWidget->setHorizontalHeaderItem(5, new QTableWidgetItem("Loss %"));

    // --- Gross & Net Weight display ---
    {
        // Cell (4,4) = final polish return (gross weight)
        QTableWidgetItem *grossItem = ui->goldDetailTableWidget->item(4, 4);
        double grossWt = (grossItem && !grossItem->text().isEmpty())
                             ? grossItem->text().toDouble()
                             : 0.0;

        // Total used diamond + stone + other weight (already calculated above)
        double totalUsedDSO = 0.0;
        {
            auto getUsedWeight = [this](int row) -> double {
                QTableWidget *tbl = ui->diamondAndStoneDetailTableWidget;
                auto val = [&](int c) -> double {
                    QTableWidgetItem *it = tbl->item(row, c);
                    return (it && !it->text().isEmpty()) ? it->text().toDouble() : 0.0;
                };
                return val(2) - val(4) - val(6);
            };
            totalUsedDSO = getUsedWeight(0) + getUsedWeight(1) + getUsedWeight(2);
        }

        // Net weight = gross - (diamond + stone + other)
        double netWt = grossWt - totalUsedDSO;
        if (netWt < 0.0)
            netWt = 0.0; // safety clamp

        // --- Update LineEdits ---
        ui->grossWtLineEdit->setText(QString::number(grossWt, 'f', 3));
        ui->netWtLineEdit->setText(QString::number(netWt, 'f', 3));

        qDebug() << "Gross Wt:" << grossWt
                 << " | Used DSO:" << totalUsedDSO
                 << " | Net Wt:" << netWt;
    }

}

void JobSheet::updateDiamondTotals()
{
    QString jobNo = ui->jobNoLineEdit->text().trimmed();
    if (jobNo.isEmpty())
        return;

    // ✅ Open DB
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "diamond_total_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath())
                         .filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::warning(this, "DB Error", db.lastError().text());
        return;
    }

    // ✅ Helper lambda: read total pcs & wt from a given column name
    auto getTotals = [&](const QString &column) -> QPair<int, double> {
        int totalPcs = 0;
        double totalWt = 0.0;

        QSqlQuery q(db);
        q.prepare(QString("SELECT \"%1\" FROM jobsheet_detail WHERE job_no = ?").arg(column));
        q.addBindValue(jobNo);

        if (q.exec() && q.next()) {
            QString json = q.value(0).toString();
            if (!json.isEmpty()) {
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && doc.isArray()) {
                    QJsonArray arr = doc.array();
                    for (const QJsonValue &v : arr) {
                        if (!v.isObject()) continue;
                        QJsonObject obj = v.toObject();
                        totalPcs += obj["pcs"].toInt();
                        totalWt += obj["wt"].toString().toDouble();
                    }
                }
            }
        }

        return { totalPcs, totalWt };
    };

    // ✅ List of all columns with mapping to row & col
    struct Entry { QString col; int row; int pcsCol; int wtCol; };
    QList<Entry> entries = {
                            // Diamond
                            { "diamond_issue",  0, 1, 2 },
                            { "diamond_return", 0, 3, 4 },
                            { "diamond_broken", 0, 5, 6 },

                            // Stone
                            { "stone_issue",  1, 1, 2 },
                            { "stone_return", 1, 3, 4 },
                            { "stone_broken", 1, 5, 6 },

                            // Other
                            { "other_issue",  2, 1, 2 },
                            { "other_return", 2, 3, 4 },
                            { "other_broken", 2, 5, 6 },
                            };

    // ✅ Loop and fill
    for (const auto &e : entries) {
        auto [pcs, wt] = getTotals(e.col);

        QTableWidgetItem *pcsItem = ui->diamondAndStoneDetailTableWidget->item(e.row, e.pcsCol);
        if (!pcsItem) {
            pcsItem = new QTableWidgetItem();
            ui->diamondAndStoneDetailTableWidget->setItem(e.row, e.pcsCol, pcsItem);
        }
        pcsItem->setText(QString::number(pcs));

        QTableWidgetItem *wtItem = ui->diamondAndStoneDetailTableWidget->item(e.row, e.wtCol);
        if (!wtItem) {
            wtItem = new QTableWidgetItem();
            ui->diamondAndStoneDetailTableWidget->setItem(e.row, e.wtCol, wtItem);
        }
        wtItem->setText(QString::number(wt, 'f', 3));
    }

    db.close();
    QSqlDatabase::removeDatabase("diamond_total_conn");


    // ✅ --- Calculate net weights (col2 - col4 - col6) ---
    auto getNetWeight = [this](int row) -> double {
        QTableWidget *tbl = ui->diamondAndStoneDetailTableWidget;

        auto getVal = [&](int col) -> double {
            QTableWidgetItem *item = tbl->item(row, col);
            return (item && !item->text().isEmpty()) ? item->text().toDouble() : 0.0;
        };

        double issue  = getVal(2);
        double ret    = getVal(4);
        double broken = getVal(6);

        return issue - ret - broken;
    };

    // ✅ Compute for each category
    double diamondNet = getNetWeight(0);
    double stoneNet   = getNetWeight(1);
    double otherNet   = getNetWeight(2);

    // ✅ Update UI line edits
    ui->diaWtLineEdit->setText(QString::number(diamondNet, 'f', 3));
    ui->stoneWtLineEdit->setText(QString::number(stoneNet, 'f', 3));
    ui->otherWtLineEdit->setText(QString::number(otherNet, 'f', 3));


}


// call this from set_value_manuf() to enable only diamond-issue cells (0,1) and (0,2)
void JobSheet::setupDiamondIssueClicks()
{
    // Use UniqueConnection to avoid multiple identical connects if set_value_manuf() runs again
    connect(ui->diamondAndStoneDetailTableWidget, &QTableWidget::cellClicked, this,
            [this](int row, int col) {
                // Only allow diamond issue cells for step 1
                if (col == 0)
                    return;

                QTableWidgetItem *item = ui->diamondAndStoneDetailTableWidget->item(row, col);
                if (!item) {
                    item = new QTableWidgetItem();
                    ui->diamondAndStoneDetailTableWidget->setItem(row, col, item);
                }

                // get job no
                QString jobNo = ui->jobNoLineEdit->text().trimmed();
                if (jobNo.isEmpty()) {
                    QMessageBox::warning(this, "Missing Job No",
                                         "Please enter or select a valid Job Number before adding diamond data.");
                    return;
                }

                // lazy init popup
                if (!newDiamonIssueRetBro) {
                    newDiamonIssueRetBro = new DiamonIssueRetBro(this);
                    newDiamonIssueRetBro->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
                    newDiamonIssueRetBro->setAttribute(Qt::WA_DeleteOnClose, false);

                    // hide handling
                    connect(newDiamonIssueRetBro, &DiamonIssueRetBro::menuHidden, this, [this]() {
                        qApp->removeEventFilter(this);
                        diamondMenuVisible = false;
                    });

                    // when popup saves, update cells & totals
                    // ✅ When popup saves data, update that row & refresh totals
                    connect(newDiamonIssueRetBro, &DiamonIssueRetBro::valuesUpdated, this,
                            [this](int r, const QVariantMap &vals) {
                                auto setVal = [this](int rr, int cc, const QString &val) {
                                    QTableWidgetItem *cell = ui->diamondAndStoneDetailTableWidget->item(rr, cc);
                                    if (!cell) {
                                        cell = new QTableWidgetItem();
                                        ui->diamondAndStoneDetailTableWidget->setItem(rr, cc, cell);
                                    }
                                    cell->setText(val);
                                };

                                // 🧩 Dynamically handle all column types
                                if (vals.contains("issue_pcs"))  setVal(r, 1, vals["issue_pcs"].toString());
                                if (vals.contains("issue_wt"))   setVal(r, 2, vals["issue_wt"].toString());
                                if (vals.contains("return_pcs")) setVal(r, 3, vals["return_pcs"].toString());
                                if (vals.contains("return_wt"))  setVal(r, 4, vals["return_wt"].toString());
                                if (vals.contains("broken_pcs")) setVal(r, 5, vals["broken_pcs"].toString());
                                if (vals.contains("broken_wt"))  setVal(r, 6, vals["broken_wt"].toString());

                                // ✅ Always refresh all totals after DB update
                                updateDiamondTotals();
                            });

                }

                // pass context so popup can save to DB correctly
                newDiamonIssueRetBro->setContext(row, col, jobNo);

                // position popup next to cell
                QRect cellRect = ui->diamondAndStoneDetailTableWidget->visualItemRect(item);
                QPoint globalPos = ui->diamondAndStoneDetailTableWidget->viewport()->mapToGlobal(cellRect.bottomLeft());

                // toggle show/hide
                if (newDiamonIssueRetBro->isVisible()) {
                    newDiamonIssueRetBro->hide();
                    diamondMenuVisible = false;
                    qApp->removeEventFilter(this);
                } else {
                    newDiamonIssueRetBro->move(globalPos);
                    newDiamonIssueRetBro->show();
                    newDiamonIssueRetBro->raise();
                    diamondMenuVisible = true;
                    qApp->installEventFilter(this);
                }
            // }, Qt::UniqueConnection);
    });
}

void JobSheet::handleCellSave(int row, int col)
{
    QTableWidgetItem *item = ui->goldDetailTableWidget->item(row, col);
    if (!item) return;

    QString jobNo = ui->jobNoLineEdit->text().trimmed();
    if (jobNo.isEmpty()) return;

    // ✅ Map valid stage return columns
    QString dbColumn;
    if (row == 1 && col == 4) dbColumn = "buffing_return";
    else if (row == 2 && col == 4) dbColumn = "free_polish_return";
    else if (row == 3 && col == 4) dbColumn = "setting_return";
    else if (row == 4 && col == 4) dbColumn = "final_polish_return";
    else
        return; // no DB action for other cells (dust handled by ManageGold)

    // ✅ Open SQLite connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "cell_save_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(this, "DB Error", db.lastError().text());
        return;
    }

    QString text = item->text().trimmed();
    if (text.isEmpty()) return;

    bool ok;
    double value = text.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Invalid", "Please enter a valid number.");
        item->setText("");
        db.close();
        QSqlDatabase::removeDatabase("cell_save_conn");
        return;
    }

    QString formatted = QString::number(value, 'f', 3);

    // ✅ Check if already exists
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

    // ✅ Confirm save
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
