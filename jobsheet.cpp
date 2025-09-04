#include "jobsheet.h"
#include "ui_jobsheet.h"

#include <QDir>
#include <QDebug>

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
    }

    QList<QLabel*> labels = findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label != ui->productImageLabel) {
            label->setEnabled(false);
        } else if (userRole != "designer") {
            label->setEnabled(false); // restrict image even for label if not designer
        }
    }


    set_value(jobNo);
    ui->extraNoteTextEdit->setText(R"(ACKNOWLEDGMENT OF ENTRUSTMENT
We Hereby acknowledge receipt of the following goods mentioned overleaf which you have entrusted to melus and to which IAve hold in trust for you for the following purpose and on following conditions.
(1) The goods have been entrusted to me/us for the sale purpose of being shown to intending purchasers of inspection.
(2) The Goods remain your property and Iwe acquire no right to property or interest in them till sale note signed by you is passed or till the price is paid in respect there of not with standing the fact that mention is made of the rate or price in the particulars of goods herein behind set.
(3) IWe agree not to sell or pledge, or montage or hypothecate the said goods or otherwise dea; with them in any manner till a sale note signed by you is passed or the price is paid to you. (4) The goods are to be returned to you forthwith whenever demanded back.
(5) The Goods will be at my/out risk in all respect till a sale note signed by you is passed in respecte there of or ti the price is paid to you or till the goods are returned to you and  Awe am/are responsible to you for the retum of the said goods in the same condition as I/we have received the same.
(6) Subject to Surat Jurisdiction.)");

    ui->extraNoteTextEdit->setStyleSheet("font-size: 7.3pt;");

    if (userRole == "designer") {
        connect(ui->desigNoLineEdit, &QLineEdit::returnPressed, this, &JobSheet::loadImageForDesignNo);
    }
}

JobSheet::~JobSheet()
{
    delete ui;
}

void JobSheet::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);  // Call base class

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
        QPixmap pixmap(fullPath);
        ui->productImageLabel->setScaledContents(true);
        ui->productImageLabel->setPixmap(pixmap);
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
