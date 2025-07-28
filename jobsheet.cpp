#include "jobsheet.h"
#include "ui_jobsheet.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>

#include <QDebug>

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
        SELECT sellerId, partyId, jobNo, orderNo, clientId, orderDate, deliveryDate,
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

    if (query.next()) {
        ui->jobIssuLineEdit->setText(query.value("sellerId").toString());
        ui->orderPartyLineEdit->setText(query.value("partyId").toString());
        ui->jobNoLineEdit->setText(query.value("jobNo").toString());
        ui->orderNoLineEdit->setText(query.value("orderNo").toString());
        ui->clientIdLineEdit->setText(query.value("clientId").toString());

        // Dates
        QDate orderDate = QDate::fromString(query.value("orderDate").toString(), "yyyy-MM-dd");
        ui->dateOrderDateEdit->setDate(orderDate);

        QDate deliveryDate = QDate::fromString(query.value("deliveryDate").toString(), "yyyy-MM-dd");
        ui->deliDateDateEdit->setDate(deliveryDate);

        // Product details
        ui->itemDesignLineEdit->setText(QString::number(query.value("productPis").toInt()));
        ui->desigNoLineEdit->setText(query.value("designNo1").toString());
        ui->purityLineEdit->setText(query.value("metalPurity").toString());
        ui->metColLineEdit->setText(query.value("metalColor").toString());


        // Size-related
        ui->sizeNoLineEdit->setText(QString::number(query.value("sizeNo").toDouble()));
        ui->MMLineEdit->setText(QString::number(query.value("sizeMM").toDouble()));
        ui->lengthLineEdit->setText(QString::number(query.value("length").toDouble()));
        ui->widthLineEdit->setText(QString::number(query.value("width").toDouble()));
        ui->heightLineEdit->setText(QString::number(query.value("height").toDouble()));

        // ✅ Load image if image1path exists
        QString imagePath = query.value("image1path").toString();
        QString fullPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/" + imagePath);

        QPixmap pixmap(fullPath);
        if (!imagePath.isNull()) {
            ui->productImageLabel->setScaledContents(true);
            ui->productImageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            ui->productImageLabel->setPixmap(pixmap);
        }
        else {
            qDebug() << "⚠️ Could not load image for design from:" << fullPath;
        }

    } else {
        qDebug() << "No record found for jobNo:" << jobNo;
    }

    // Extract designNo
    QString designNo = query.value("designNo1").toString();
    ui->desigNoLineEdit->setText(designNo);

    // Switch to image DB
    QSqlDatabase imageDb = QSqlDatabase::addDatabase("QSQLITE", "design_info_conn");
    imageDb.setDatabaseName("database/mega_mine_image.db");

    if (!imageDb.open()) {
        qDebug() << "❌ Could not open image database:" << imageDb.lastError().text();
        return;
    }

    // Query diamond & stone info
    QSqlQuery imgQuery(imageDb);
    imgQuery.prepare("SELECT diamond, stone FROM image_data WHERE design_no = :designNo");
    imgQuery.bindValue(":designNo", designNo);

    if (!imgQuery.exec()) {
        qDebug() << "❌ Failed to query diamond & stone:" << imgQuery.lastError().text();
        imageDb.close();
        QSqlDatabase::removeDatabase("design_info_conn");
        return;
    }

    QTableWidget* table = ui->diaAndStoneForDesignTableWidget;
    table->setRowCount(0); // Clear existing
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Type", "Name", "Quantity", "Size (MM)"});

    auto parseAndAddRows = [&](const QString& jsonStr, const QString& typeLabel) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            qDebug() << "⚠️ Failed to parse JSON for" << typeLabel << ":" << parseError.errorString();
            return;
        }

        QJsonArray array = doc.array();
        for (const QJsonValue& value : array) {
            if (!value.isObject()) continue;

            QJsonObject obj = value.toObject();
            QString name = obj.value("type").toString();
            QString qty = obj.value("quantity").toString();
            QString size = obj.value("sizeMM").toString();

            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(typeLabel));
            table->setItem(row, 1, new QTableWidgetItem(name));
            table->setItem(row, 2, new QTableWidgetItem(qty));
            table->setItem(row, 3, new QTableWidgetItem(size));
        }
    };

    if (imgQuery.next()) {
        QString diamondJson = imgQuery.value("diamond").toString();
        QString stoneJson = imgQuery.value("stone").toString();

        parseAndAddRows(diamondJson, "Diamond");
        parseAndAddRows(stoneJson, "Stone");

        table->resizeColumnsToContents();
    } else {
        qDebug() << "ℹ️ No diamond/stone data found for design:" << designNo;
    }

    imageDb.close();
    QSqlDatabase::removeDatabase("design_info_conn");


    db.close();  // Always close the DB
    QSqlDatabase::removeDatabase("set_jobsheet");
}

void JobSheet::loadImageForDesignNo()
{
    QString designNo = ui->desigNoLineEdit->text().trimmed();
    if (designNo.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Design number is empty.");
        return;
    }

    // Set working directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // Open the image database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "image_conn");
    db.setDatabaseName("database/mega_mine_image.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error", "Failed to open image database:\n" + db.lastError().text());
        return;
    }

    QSqlQuery query(db);
    query.prepare("SELECT image_path FROM image_data WHERE design_no = :designNo");
    query.bindValue(":designNo", designNo);

    if (!query.exec()) {
        QMessageBox::critical(this, "Query Error", "Failed to execute query:\n" + query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("image_conn");
        return;
    }

    if (query.next()) {
        QString imagePath = query.value("image_path").toString();
        QString fullPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/" + imagePath);

        QPixmap pixmap(fullPath);
        if (!pixmap.isNull()) {
            ui->productImageLabel->setPixmap(pixmap.scaled(ui->productImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

            // ✅ Save to OrderBook-Detail table
            saveDesignNoAndImagePath(designNo, imagePath);

        } else {
            QMessageBox::warning(this, "Image Error", "Image not found at path:\n" + fullPath);
        }
    } else {
        QMessageBox::information(this, "Not Found", "No image found for design number: " + designNo);
    }

    QSqlQuery detailsQuery(db);
    detailsQuery.prepare("SELECT diamond, stone FROM image_data WHERE design_no = :designNo");
    detailsQuery.bindValue(":designNo", designNo);

    if (!detailsQuery.exec()) {
        QMessageBox::critical(this, "Query Error", "Failed to fetch diamond and stone data:\n" + detailsQuery.lastError().text());
    } else if (detailsQuery.next()) {
        QString diamondJson = detailsQuery.value("diamond").toString();
        QString stoneJson = detailsQuery.value("stone").toString();

        QTableWidget* table = ui->diaAndStoneForDesignTableWidget;
        table->setRowCount(0); // clear previous
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"Type", "Name", "Quantity", "Size (MM)"});

        auto parseAndAddRows = [&](const QString& jsonStr, const QString& typeLabel) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isArray())
                return;

            QJsonArray array = doc.array();
            for (const QJsonValue& value : array) {
                if (!value.isObject()) continue;

                QJsonObject obj = value.toObject();
                QString name = obj.value("type").toString();
                QString qty = obj.value("quantity").toString();
                QString size = obj.value("sizeMM").toString();

                int row = table->rowCount();
                table->insertRow(row);
                table->setItem(row, 0, new QTableWidgetItem(typeLabel)); // "Diamond" or "Stone"
                table->setItem(row, 1, new QTableWidgetItem(name));
                table->setItem(row, 2, new QTableWidgetItem(qty));
                table->setItem(row, 3, new QTableWidgetItem(size));
            }
        };

        parseAndAddRows(diamondJson, "Diamond");
        parseAndAddRows(stoneJson, "Stone");

        table->resizeColumnsToContents(); // optional
    }


    db.close();
    QSqlDatabase::removeDatabase("image_conn");
}

void JobSheet::saveDesignNoAndImagePath(const QString &designNo, const QString &imagePath)
{
    QString jobNo = ui->jobNoLineEdit->text().trimmed();
    if (jobNo.isEmpty()) {
        QMessageBox::warning(this, "Missing Data", "Job No is missing. Cannot save image path.");
        return;
    }

    // Set working directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "save_design_image");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        QMessageBox::critical(this, "DB Error", "Failed to open orderbook DB:\n" + db.lastError().text());
        return;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE "OrderBook-Detail"
        SET designNo1 = :designNo, image1path = :imagePath
        WHERE jobNo = :jobNo
    )");

    query.bindValue(":designNo", designNo);
    query.bindValue(":imagePath", imagePath);
    query.bindValue(":jobNo", jobNo);

    if (!query.exec()) {
        QMessageBox::critical(this, "Query Error", "Failed to update OrderBook-Detail:\n" + query.lastError().text());
    } else {
        qDebug() << "✅ Design number and image path updated for jobNo:" << jobNo;
    }

    db.close();
    QSqlDatabase::removeDatabase("save_design_image");
}
