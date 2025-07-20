#include "jobsheet.h"
#include "ui_jobsheet.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>

JobSheet::JobSheet(QWidget *parent, const QString &jobNo)
    : QDialog(parent),
    ui(new Ui::JobSheet)
{
    ui->setupUi(this);

    setWindowTitle("Job Sheet");
    setMinimumSize(100, 100);
    setMaximumSize(1410, 910);
    resize(1410, 910);

    QList<QLineEdit*> lineEdits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        if (edit != ui->desigNoLineEdit) {
            edit->setReadOnly(true);  // Optional: light gray to indicate read-only
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

    connect(ui->desigNoLineEdit, &QLineEdit::returnPressed, this, &JobSheet::loadImageForDesignNo);

}

JobSheet::~JobSheet()
{
    delete ui;
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
        if (!pixmap.isNull()) {
            ui->productImageLabel->setPixmap(pixmap.scaled(ui->productImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            qDebug() << "⚠️ Could not load image for design from:" << fullPath;
        }

    } else {
        qDebug() << "No record found for jobNo:" << jobNo;
    }

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
