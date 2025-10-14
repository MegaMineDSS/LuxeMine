#include "managegold.h"
#include "ui_managegold.h"

#include <QDoubleValidator>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QLineEdit>
#include <QHeaderView>

ManageGold::ManageGold(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ManageGold)
    , currentMode(Filling)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    // ui->typeComboBox->addItems({"gold", "solder"});

    setStyleSheet("QDialog { background-color: #84bbe8; }");

    QDoubleValidator *validator = new QDoubleValidator(0.0, 999999.999, 3, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    ui->weightLineEdit->setValidator(validator);

    connect(ui->weightLineEdit, &QLineEdit::editingFinished, this, [this]() {
        QString text = ui->weightLineEdit->text().trimmed();
        if (!text.isEmpty()) {
            double value = text.toDouble();
            ui->weightLineEdit->setText(QString::number(value, 'f', 3));
        }
    });

    loadHistory();
}

ManageGold::~ManageGold()
{
    delete ui;
}

void ManageGold::setMode(Mode mode)
{
    currentMode = mode;

    if (mode == Filling)
        ui->pushButton->setText("Filling Gold");
    else if (mode == Returning)
        ui->pushButton->setText("Returning Gold");
    else if (mode == Dust)
        ui->pushButton->setText("Add Dust Weight");

    // ✅ Limit combo box for Dust
    if (mode == Dust) {
        ui->typeComboBox->clear();
        ui->typeComboBox->addItem("Dust");
    } else {
        ui->typeComboBox->clear();
        ui->typeComboBox->addItems({"gold", "solder"});
    }

    loadHistory();
}


bool ManageGold::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // If the click is outside the dialog geometry
        if (!this->geometry().contains(mouseEvent->globalPosition().toPoint())) {
            this->hide();
            return true; // consume the event
        }
    }
    return QDialog::eventFilter(watched, event);
}

void ManageGold::hideEvent(QHideEvent *event)
{
    qApp->removeEventFilter(this);
    ui->stackedWidget->setCurrentIndex(0);
    loadHistory();
    emit menuHidden();
    QDialog::hideEvent(event);
}

void ManageGold::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void ManageGold::on_issueAddPushButton_clicked()
{
    QString weight = ui->weightLineEdit->text().trimmed();
    if (weight.isEmpty()) {
        QMessageBox::warning(this, "Empty Field", "Enter weight");
        return;
    }

    QString type = ui->typeComboBox->currentText().trimmed();
    QString dateTime = QDateTime::currentDateTime().toString("dd-MM-yyyy HH:mm:ss");

    // Build JSON object
    QJsonObject newEntry;
    newEntry["type"] = type;
    newEntry["weight"] = weight;
    newEntry["date_time"] = dateTime;

    QString jobNo;
    if (parentWidget()) {
        QLineEdit *jobNoLineEdit = parentWidget()->findChild<QLineEdit*>("jobNoLineEdit");
        if (jobNoLineEdit)
            jobNo = jobNoLineEdit->text().trimmed();
    }

    if (jobNo.isEmpty()) {
        QMessageBox::warning(this, "Missing Data", "Job No not found.");
        return;
    }

    QString columnName;
    if (currentMode == Filling)
        columnName = "Filling_Issue";
    else if (currentMode == Returning)
        columnName = "Filling_Return";
    else
        columnName = "filling_dust";   // ✅ new column


    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "gold_mode");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        QMessageBox::critical(this, "DB Error", db.lastError().text());
        return;
    }

    QJsonArray arr;

    // Fetch existing JSON
    QSqlQuery selectQuery(db);
    selectQuery.prepare(QString("SELECT %1 FROM jobsheet_detail WHERE job_no = ?").arg(columnName));
    selectQuery.addBindValue(jobNo);
    if (selectQuery.exec() && selectQuery.next()) {
        QString existingJson = selectQuery.value(0).toString();
        if (!existingJson.isEmpty()) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(existingJson.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError && doc.isArray())
                arr = doc.array();
        }
    }

    // Append new entry
    arr.append(newEntry);
    QString updatedJson = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));

    QSqlQuery updateQuery(db);
    updateQuery.prepare(QString("UPDATE jobsheet_detail SET %1 = ? WHERE job_no = ?").arg(columnName));
    updateQuery.addBindValue(updatedJson);
    updateQuery.addBindValue(jobNo);
    if (!updateQuery.exec()) {
        QMessageBox::critical(this, "Update Error", updateQuery.lastError().text());
    }

    db.close();
    QSqlDatabase::removeDatabase("gold_mode");

    QMessageBox::information(this, "Success", "Data saved successfully.");
    loadHistory();
}

void ManageGold::loadHistory()
{
    QString jobNo;
    if (parentWidget()) {
        QLineEdit *jobNoLineEdit = parentWidget()->findChild<QLineEdit*>("jobNoLineEdit");
        if (jobNoLineEdit)
            jobNo = jobNoLineEdit->text().trimmed();
    }

    QString columnName;
    if (currentMode == Filling)
        columnName = "Filling_Issue";
    else if (currentMode == Returning)
        columnName = "Filling_Return";
    else
        columnName = "filling_dust";

    double totalWeight = 0.0;

    ui->fillingIssueTableWidget->clear();
    ui->fillingIssueTableWidget->setRowCount(0);
    ui->fillingIssueTableWidget->setColumnCount(3);
    ui->fillingIssueTableWidget->setHorizontalHeaderLabels({"Type", "Weight", "Date Time"});

    if (!jobNo.isEmpty()) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "gold_read");
        QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
        db.setDatabaseName(dbPath);

        if (db.open()) {
            QSqlQuery query(db);
            query.prepare(QString("SELECT %1 FROM jobsheet_detail WHERE job_no = ?").arg(columnName));
            query.addBindValue(jobNo);
            if (query.exec() && query.next()) {
                QString jsonStr = query.value(0).toString();
                if (!jsonStr.isEmpty()) {
                    QJsonParseError parseError;
                    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                        QJsonArray arr = doc.array();
                        ui->fillingIssueTableWidget->setRowCount(arr.size());
                        for (int i = 0; i < arr.size(); ++i) {
                            QJsonObject obj = arr[i].toObject();
                            ui->fillingIssueTableWidget->setItem(i, 0, new QTableWidgetItem(obj["type"].toString()));
                            ui->fillingIssueTableWidget->setItem(i, 1, new QTableWidgetItem(obj["weight"].toString()));
                            ui->fillingIssueTableWidget->setItem(i, 2, new QTableWidgetItem(obj["date_time"].toString()));
                            totalWeight += obj["weight"].toString().toDouble();
                        }
                    }
                }
            }
        }
        db.close();
        QSqlDatabase::removeDatabase("gold_read");
    }

    if (ui->fillingIssueTableWidget->rowCount() == 0) {
        ui->fillingIssueTableWidget->setRowCount(1);
        ui->fillingIssueTableWidget->setItem(0, 0, new QTableWidgetItem("No history found"));
        ui->fillingIssueTableWidget->setSpan(0, 0, 1, 3);
    }

    emit totalWeightCalculated(totalWeight);
}
