#include "diamonissueretbro.h"
#include "ui_diamonissueretbro.h"
#include <QMessageBox>
#include <QSqlError>

DiamonIssueRetBro::DiamonIssueRetBro(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DiamonIssueRetBro)
{
    ui->setupUi(this);

    // Default selection
    ui->diamondRadioButton->setChecked(true);
    ui->sizeComboBox->setEnabled(true);

    connect(ui->diamondRadioButton, &QRadioButton::toggled, this, &DiamonIssueRetBro::onRadioChanged);
    connect(ui->stoneRadioButton, &QRadioButton::toggled, this, &DiamonIssueRetBro::onRadioChanged);
    connect(ui->otherRadioButton, &QRadioButton::toggled, this, &DiamonIssueRetBro::onRadioChanged);
    connect(ui->typeComboBox, &QComboBox::currentTextChanged, this, &DiamonIssueRetBro::onTypeChanged);
    connect(ui->addPushButton, &QPushButton::clicked, this, &DiamonIssueRetBro::onSaveClicked);


    ui->historyTableWidget->setColumnCount(4);
    ui->historyTableWidget->setHorizontalHeaderLabels({"Type", "Size", "Pcs", "Weight"});
    ui->historyTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->historyTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->historyTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);


    loadTypeOptions(); // initial
    loadHistoryForCurrentContext();
}

DiamonIssueRetBro::~DiamonIssueRetBro()
{
    delete ui;
}

void DiamonIssueRetBro::hideEvent(QHideEvent *event)
{
    qApp->removeEventFilter(this);
    ui->stackedWidget->setCurrentIndex(0);
    // loadHistory();
    ui->sizeComboBox->clear();
    ui->typeComboBox->clear();
    ui->quantityLineEdit->clear();
    ui->weightLineEdit->clear();
    emit menuHidden();
    QDialog::hideEvent(event);
}

void DiamonIssueRetBro::setContext(int row, int col, const QString &jobNo)
{
    currentRow = row;
    currentCol = col;
    currentJobNo = jobNo;

    // Determine mode based on column
    if (col == 1 || col == 2)
        currentMode = "issue";
    else if (col == 3 || col == 4)
        currentMode = "return";
    else if (col == 5 || col == 6)
        currentMode = "broken";
    else
        currentMode.clear();

    // Set radio button automatically
    if (row == 0)
        ui->diamondRadioButton->setChecked(true);
    else if (row == 1)
        ui->stoneRadioButton->setChecked(true);
    else
        ui->otherRadioButton->setChecked(true);

    // Load correct JSON data into table
    loadHistoryForCurrentContext();
}


//
// 🧭 Step 1: Handle Radio Change
//
void DiamonIssueRetBro::onRadioChanged()
{
    ui->typeComboBox->clear();
    ui->sizeComboBox->clear();

    if (ui->diamondRadioButton->isChecked()) {
        ui->sizeComboBox->setEnabled(true);
    } else if (ui->stoneRadioButton->isChecked()) {
        ui->sizeComboBox->setEnabled(true);
    } else { // Other
        ui->typeComboBox->setEditable(true);
        ui->sizeComboBox->setEnabled(false);
    }

    loadTypeOptions();
}

//
// 🧭 Step 2: Load Type Combo Values
//
void DiamonIssueRetBro::loadTypeOptions()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "diamond_popup_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::warning(this, "DB Error", db.lastError().text());
        return;
    }

    QSqlQuery query(db);
    QStringList types;

    if (ui->diamondRadioButton->isChecked()) {
        // Fetch distinct shapes from Fancy_diamond and add "Round"
        query.exec("SELECT DISTINCT shape FROM Fancy_diamond");
        types << "Round";
        while (query.next())
            types << query.value(0).toString();
    } else if (ui->stoneRadioButton->isChecked()) {
        query.exec("SELECT DISTINCT shape FROM stones");
        while (query.next())
            types << query.value(0).toString();
    } else {
        // Other: editable text
        ui->typeComboBox->setEditable(true);
    }

    ui->typeComboBox->addItems(types);
    db.close();
    QSqlDatabase::removeDatabase("diamond_popup_conn");
}

//
// 🧭 Step 3: Load Size Combo Values based on Type
//
void DiamonIssueRetBro::onTypeChanged()
{
    ui->sizeComboBox->clear();

    QString type = ui->typeComboBox->currentText();
    if (type.isEmpty())
        return;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "diamond_popup_conn2");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) return;

    QSqlQuery query(db);

    if (ui->diamondRadioButton->isChecked()) {
        if (type == "Round") {
            query.prepare("SELECT sizeMM FROM Round_diamond ORDER BY sizeMM ASC");
        } else {
            query.prepare("SELECT DISTINCT sizeMM FROM Fancy_diamond WHERE shape = ? ORDER BY sizeMM ASC");
            query.addBindValue(type);
        }
    } else if (ui->stoneRadioButton->isChecked()) {
        query.prepare("SELECT DISTINCT sizeMM FROM stones WHERE shape = ? ORDER BY sizeMM ASC");
        query.addBindValue(type);
    } else {
        ui->sizeComboBox->setEnabled(false);
    }

    if (query.exec()) {
        while (query.next())
            ui->sizeComboBox->addItem(query.value(0).toString());
    }

    db.close();
    QSqlDatabase::removeDatabase("diamond_popup_conn2");
}

//
// 🧭 Step 4: Save JSON to DB
//
void DiamonIssueRetBro::onSaveClicked()
{
    QString type = ui->typeComboBox->currentText().trimmed();
    QString size = ui->sizeComboBox->currentText().trimmed();

    // Get input values
    QString qtyStr = ui->quantityLineEdit->text().trimmed();
    QString wtStr = ui->weightLineEdit->text().trimmed();

    // Validate quantity
    bool okQty;
    int qty = qtyStr.toInt(&okQty);
    if (!okQty || qty <= 0) {
        QMessageBox::warning(this, "Invalid Quantity", "Please enter a valid integer quantity greater than 0.");
        return;
    }

    // Validate weight
    bool okWt;
    double wt = wtStr.toDouble(&okWt);
    if (!okWt || wt <= 0.0) {
        QMessageBox::warning(this, "Invalid Weight", "Please enter a valid weight greater than 0.");
        return;
    }

    // Round/format weight
    QString formattedWt = QString::number(wt, 'f', 3); // always 3 decimals

    // Determine category
    QString category;
    if (ui->diamondRadioButton->isChecked()) category = "diamond";
    else if (ui->stoneRadioButton->isChecked()) category = "stone";
    else category = "other";

    // Build JSON entry
    QJsonObject entry;
    entry["type"] = type;
    entry["size"] = size;
    entry["category"] = category;
    entry["pcs"] = qty;
    entry["wt"] = formattedWt;

    // Save JSON into database
    saveToDatabase(entry);

    // Emit signal to update main table
    QVariantMap vals;
    vals["issue_pcs"] = QString::number(qty);
    vals["issue_wt"]  = formattedWt;

    emit valuesUpdated(currentRow, vals);
    emit menuHidden();
    close();
}


//
// 🧭 Step 5: Save JSON Entry to DB
//
void DiamonIssueRetBro::saveToDatabase(const QJsonObject &entry)
{
    // QString colName;

    QString baseName;

    // 🧭 Decide base column using radio button, not row
    if (ui->diamondRadioButton->isChecked())
        baseName = "diamond";
    else if (ui->stoneRadioButton->isChecked())
        baseName = "stone";
    else if (ui->otherRadioButton->isChecked())
        baseName = "other";
    else
        return; // no valid selection

    // 🧭 Decide suffix based on mode (issue / return / broken)
    QString colName;
    if (currentMode == "issue")
        colName = baseName + "_issue";
    else if (currentMode == "return")
        colName = baseName + "_return";
    else if (currentMode == "broken")
        colName = baseName + "_broken";  // optional, if you add broken later
    else
        return;


    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "diamond_save_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ DB open failed:" << db.lastError().text();
        return;
    }

    // // Ensure row exists
    // QSqlQuery ensure(db);
    // ensure.prepare("INSERT OR IGNORE INTO jobsheet_detail (job_no) VALUES (?)");
    // ensure.addBindValue(currentJobNo);
    // ensure.exec();

    // Fetch existing JSON
    QSqlQuery q(db);
    q.prepare(QString("SELECT \"%1\" FROM jobsheet_detail WHERE job_no = ?").arg(colName));
    q.addBindValue(currentJobNo);
    QString existing;
    if (q.exec() && q.next())
        existing = q.value(0).toString();

    QJsonArray arr;
    if (!existing.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(existing.toUtf8());
        if (doc.isArray()) arr = doc.array();
    }

    arr.append(entry);
    QString jsonStr = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));

    q.prepare(QString("UPDATE jobsheet_detail SET \"%1\" = ? WHERE job_no = ?").arg(colName));
    q.addBindValue(jsonStr);
    q.addBindValue(currentJobNo);
    if (!q.exec()) {
        qDebug() << "❌ Update failed:" << q.lastError().text();
    } else {
        qDebug() << "✅ Updated" << colName;
    }

    db.close();
    QSqlDatabase::removeDatabase("diamond_save_conn");
}

// void DiamonIssueRetBro::onCancelClicked()
// {
//     emit menuHidden();
//     close();
// }

void DiamonIssueRetBro::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void DiamonIssueRetBro::loadHistoryForCurrentContext()
{
    if (currentJobNo.isEmpty()) return;

    // 🧭 Step 1: Determine base column from radio buttons
    QString baseName;
    if (ui->diamondRadioButton->isChecked())
        baseName = "diamond";
    else if (ui->stoneRadioButton->isChecked())
        baseName = "stone";
    else if (ui->otherRadioButton->isChecked())
        baseName = "other";
    else
        return;

    // 🧭 Step 2: Determine suffix based on mode
    QString colName;
    if (currentMode == "issue")
        colName = baseName + "_issue";
    else if (currentMode == "return")
        colName = baseName + "_return";
    else if (currentMode == "broken")
        colName = baseName + "_broken";
    else
        return;

    // 🧭 Step 3: Open DB connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "diamond_history_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ DB Open Failed:" << db.lastError().text();
        return;
    }

    QSqlQuery q(db);
    q.prepare(QString("SELECT \"%1\" FROM jobsheet_detail WHERE job_no = ?").arg(colName));
    q.addBindValue(currentJobNo);

    QString jsonStr;
    if (q.exec() && q.next())
        jsonStr = q.value(0).toString();

    db.close();
    QSqlDatabase::removeDatabase("diamond_history_conn");

    // 🧭 Step 4: Clear and show history
    ui->historyTableWidget->setRowCount(0);
    if (jsonStr.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    int row = 0;
    for (const QJsonValue &val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        ui->historyTableWidget->insertRow(row);
        ui->historyTableWidget->setItem(row, 0, new QTableWidgetItem(obj["type"].toString()));
        ui->historyTableWidget->setItem(row, 1, new QTableWidgetItem(obj["size"].toString()));

        QString pcsStr = obj["pcs"].isString() ? obj["pcs"].toString() :
                             QString::number(obj["pcs"].toInt());
        QString wtStr  = obj["wt"].isString() ? obj["wt"].toString() :
                            QString::number(obj["wt"].toDouble(), 'f', 3);

        ui->historyTableWidget->setItem(row, 2, new QTableWidgetItem(pcsStr));
        ui->historyTableWidget->setItem(row, 3, new QTableWidgetItem(wtStr));
        row++;
    }
}

