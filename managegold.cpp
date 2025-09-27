#include "managegold.h"
#include "ui_managegold.h"

#include <QDoubleValidator>

ManageGold::ManageGold(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ManageGold)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->typeComboBox->addItems( { "gold", "solder" } );

    setStyleSheet("QDialog { background-color: #84bbe8; }");

    QDoubleValidator *validator = new QDoubleValidator(0.0, 999999.999, 3, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    ui->weightLineEdit->setValidator(validator);

    connect(ui->weightLineEdit, &QLineEdit::editingFinished, this, [this]() {
        QString text = ui->weightLineEdit->text().trimmed();
        if (!text.isEmpty()) {
            double value = text.toDouble();
            ui->weightLineEdit->setText(QString::number(value, 'f', 3)); // always 3 decimals
        }
    });

    loadFillingIssueHistory();
}

ManageGold::~ManageGold()
{
    delete ui;
}

void ManageGold::hideEvent(QHideEvent *event)
{
    ui->stackedWidget->setCurrentIndex(0);

    // ✅ Reuse the same function
    loadFillingIssueHistory();

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
        QMessageBox::warning(this, "Empty Field", "Enter weight" );
        return;
    }

    QString type = ui->typeComboBox->currentText().trimmed();
    QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // Build JSON object
    QJsonObject newEntry;
    newEntry["type"] = type;
    newEntry["weight"] = weight;
    newEntry["date_time"] = dateTime;

    // ---- Get current Job No ----
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

    // ---- DB Logic ----
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "issue_value");
    // db.setDatabaseName("database/mega_mine_image.db");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return;
    }

    QJsonArray arr;

    // Fetch existing JSON
    QSqlQuery selectQuery(db);
    selectQuery.prepare("SELECT filling_issue FROM jobsheet_detail WHERE job_no = ?");
    selectQuery.addBindValue(jobNo);
    if (selectQuery.exec() && selectQuery.next()) {
        QString existingJson = selectQuery.value(0).toString();
        if (!existingJson.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(existingJson.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                arr = doc.array();
            }
        }
    }

    // Append new object
    arr.append(newEntry);
    QJsonDocument doc(arr);
    QString updatedJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // Try update first
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE jobsheet_detail SET filling_issue = ? WHERE job_no = ?");
    updateQuery.addBindValue(updatedJson);
    updateQuery.addBindValue(jobNo);

    if (!updateQuery.exec() || updateQuery.numRowsAffected() == 0) {
        // If no row updated → insert
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO jobsheet_detail (job_no, filling_issue) VALUES (?, ?)");
        insertQuery.addBindValue(jobNo);
        insertQuery.addBindValue(updatedJson);

        if (!insertQuery.exec()) {
            QMessageBox::critical(this, "DB Error", "Failed to insert filling_issue: " + insertQuery.lastError().text());
            return;
        }
    }

    QMessageBox::information(this, "Success", "Filling issue saved.");
}

void ManageGold::loadFillingIssueHistory()
{
    QString jobNo;
    if (parentWidget()) {
        QLineEdit *jobNoLineEdit = parentWidget()->findChild<QLineEdit*>("jobNoLineEdit");
        if (jobNoLineEdit)
            jobNo = jobNoLineEdit->text().trimmed();
    }

    QString historyText;
    double totalWeight = 0.0;

    if (!jobNo.isEmpty()) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "issue_value_read");
        QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
        db.setDatabaseName(dbPath);

        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT filling_issue FROM jobsheet_detail WHERE job_no = ?");
            query.addBindValue(jobNo);

            if (query.exec() && query.next()) {
                QString jsonStr = query.value(0).toString();
                if (!jsonStr.isEmpty()) {
                    QJsonParseError parseError;
                    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                        QJsonArray arr = doc.array();
                        QStringList lines;
                        for (const QJsonValue &val : arr) {
                            if (val.isObject()) {
                                QJsonObject obj = val.toObject();
                                QString line = QString("%1    %2    %3")
                                                   .arg(obj["type"].toString(),
                                                        obj["weight"].toString(),
                                                        obj["date_time"].toString());
                                lines << line;

                                // ✅ add to total
                                totalWeight += obj["weight"].toString().toDouble();
                            }
                        }
                        historyText = lines.join("\n");
                    }
                }
            }
        }
        db.close();
        QSqlDatabase::removeDatabase("issue_value_read");
    }

    if (!historyText.isEmpty())
        ui->label->setText(historyText);
    else
        ui->label->setText("No filling issue history.");

    // ✅ Emit total weight back to JobSheet
    emit totalWeightCalculated(totalWeight);
}

