#include "modifycatalogdialog.h"
// #include "ui_modifycatalogdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QPixmap>
#include <QTableWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QMessageBox>

#include "databaseutils.h"

ModifyCatalogDialog::ModifyCatalogDialog(const QString &designNo, QWidget *parent)
    : QDialog(parent)
    , m_designNo(designNo)
    // , ui(new Ui::ModifyCatalogDialog)
{
    setWindowTitle("Modify design " + designNo);
    resize(700, 500);

    // Basic form layout
    QVBoxLayout *main = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();
    companyEdit = new QLineEdit();
    imageTypeEdit = new QLineEdit();
    imagePathEdit = new QLineEdit();
    imagePathEdit->setReadOnly(true);
    browseBtn = new QPushButton("Browse...");
    imagePreview = new QLabel();
    imagePreview->setFixedSize(160, 120);
    imagePreview->setFrameStyle(QFrame::Box);

    QHBoxLayout *imgRow = new QHBoxLayout();
    imgRow->addWidget(imagePathEdit);
    imgRow->addWidget(browseBtn);
    imgRow->addWidget(imagePreview);

    form->addRow("Company Name", companyEdit);
    form->addRow("Image Type", imageTypeEdit);
    form->addRow("Image", imgRow);

    main->addLayout(form);

    // Gold table (similar to your add screen)
    goldTable = new QTableWidget(6, 2, this);
    goldTable->setHorizontalHeaderLabels(QStringList() << "Karat" << "Weight(g)");
    goldTable->verticalHeader()->setVisible(false);
    goldTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    main->addWidget(new QLabel("Gold weights"));
    main->addWidget(goldTable);

    // Diamonds
    diamondTable = new QTableWidget(0, 3, this);
    diamondTable->setHorizontalHeaderLabels(QStringList() << "Shape" << "Size(mm)" << "PCS");
    diamondTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QPushButton *addDia = new QPushButton("Add Diamond");
    QPushButton *remDia = new QPushButton("Remove Selected Diamond");
    QHBoxLayout *diaButtons = new QHBoxLayout();
    diaButtons->addWidget(addDia);
    diaButtons->addWidget(remDia);

    main->addWidget(new QLabel("Diamonds"));
    main->addWidget(diamondTable);
    main->addLayout(diaButtons);

    // Stones
    stoneTable = new QTableWidget(0, 3, this);
    stoneTable->setHorizontalHeaderLabels(QStringList() << "Shape" << "Size(mm)" << "PCS");
    stoneTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QPushButton *addSt = new QPushButton("Add Stone");
    QPushButton *remSt = new QPushButton("Remove Selected Stone");
    QHBoxLayout *stButtons = new QHBoxLayout();
    stButtons->addWidget(addSt);
    stButtons->addWidget(remSt);

    main->addWidget(new QLabel("Stones"));
    main->addWidget(stoneTable);
    main->addLayout(stButtons);

    // note
    noteEdit = new QLineEdit();
    main->addWidget(new QLabel("Note"));
    main->addWidget(noteEdit);

    // Save / Cancel
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Save");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    btnRow->addStretch();
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    main->addLayout(btnRow);

    // connect signals
    connect(browseBtn, &QPushButton::clicked, this, &ModifyCatalogDialog::onBrowseImage);
    connect(addDia, &QPushButton::clicked, this, &ModifyCatalogDialog::onAddDiamondRow);
    connect(remDia, &QPushButton::clicked, this, &ModifyCatalogDialog::onRemoveDiamondRow);
    connect(addSt, &QPushButton::clicked, this, &ModifyCatalogDialog::onAddStoneRow);
    connect(remSt, &QPushButton::clicked, this, &ModifyCatalogDialog::onRemoveStoneRow);
    connect(saveBtn, &QPushButton::clicked, this, &ModifyCatalogDialog::onSaveClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Setup karat column
    QList<int> karats = {24, 22, 20, 18, 14, 10};
    for (int i = 0; i < karats.size(); ++i) {
        QTableWidgetItem *ki = new QTableWidgetItem(QString::number(karats[i]) + "kt");
        ki->setFlags(ki->flags() & ~Qt::ItemIsEditable);
        goldTable->setItem(i, 0, ki);
        goldTable->setItem(i, 1, new QTableWidgetItem());
    }

    // load existing data
    loadData();

    // ui->setupUi(this);
}

ModifyCatalogDialog::~ModifyCatalogDialog() = default;

bool ModifyCatalogDialog::loadData()
{
    // Get latest non-deleted record for this designNo
    const QString connName = "ui_conn";
    QSqlDatabase db;
    if (QSqlDatabase::contains(connName))
        db = QSqlDatabase::database(connName);
    else {
        db = QSqlDatabase::addDatabase("QSQLITE", connName);
        QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
        db.setDatabaseName(dbPath);
    }

    if (!db.open()) {
        qWarning() << "ModifyCatalogDialog: DB open failed";
        return false;
    }

    QSqlQuery q(db);
    q.prepare("SELECT image_path, image_type, company_name, gold_weight, diamond, stone, note FROM image_data WHERE design_no = :dn AND \"delete\" = 0 ORDER BY time DESC LIMIT 1");
    q.bindValue(":dn", m_designNo);
    if (!q.exec() || !q.next()) {
        qWarning() << "ModifyCatalogDialog: record not found for" << m_designNo;
        db.close();
        return false;
    }

    m_loadedImagePath = q.value(0).toString();
    imageTypeEdit->setText(q.value(1).toString());
    companyEdit->setText(q.value(2).toString());
    QString goldJsonStr = q.value(3).toString();
    QString diaJsonStr = q.value(4).toString();
    QString stJsonStr = q.value(5).toString();
    noteEdit->setText(q.value(6).toString());

    // set image preview
    QString preview = m_loadedImagePath;
    if (!QFile::exists(preview)) {
        QString alt = QDir(QCoreApplication::applicationDirPath()).filePath(preview);
        if (QFile::exists(alt)) preview = alt;
    }
    if (QFile::exists(preview)) {
        QPixmap pm(preview);
        imagePreview->setPixmap(pm.scaled(imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imagePathEdit->setText(m_loadedImagePath);
    } else {
        imagePreview->setPixmap(QPixmap(":/icon/placeholder.png").scaled(imagePreview->size()));
        imagePathEdit->setText(m_loadedImagePath);
    }

    // parse JSON fields
    if (!goldJsonStr.isEmpty()) {
        QJsonDocument d = QJsonDocument::fromJson(goldJsonStr.toUtf8());
        if (d.isArray()) populateGoldTableFromJson(d.array());
    }

    if (!diaJsonStr.isEmpty()) {
        QJsonDocument d2 = QJsonDocument::fromJson(diaJsonStr.toUtf8());
        if (d2.isArray()) populateDiamondTableFromJson(d2.array());
    }

    if (!stJsonStr.isEmpty()) {
        QJsonDocument d3 = QJsonDocument::fromJson(stJsonStr.toUtf8());
        if (d3.isArray()) populateStoneTableFromJson(d3.array());
    }

    db.close();
    return true;
}

void ModifyCatalogDialog::populateGoldTableFromJson(const QJsonArray &goldArray)
{
    // find 24kt row and set values; otherwise fill best-effort
    for (const QJsonValue &v : goldArray) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        QString karat = o.value("karat").toString(); // e.g. "22kt"
        QString w = o.value("weight(g)").toString();
        bool ok; double wd = w.toDouble(&ok);
        if (!ok) wd = o.value("weight(g)").toDouble();
        // find row
        for (int r = 0; r < goldTable->rowCount(); ++r) {
            QString k = goldTable->item(r, 0)->text();
            if (k == karat) {
                goldTable->item(r,1)->setText(QString::number(wd, 'f', 3));
                break;
            }
        }
    }
}

void ModifyCatalogDialog::populateDiamondTableFromJson(const QJsonArray &diamondArray)
{
    for (const QJsonValue &v : diamondArray) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        int row = diamondTable->rowCount();
        diamondTable->insertRow(row);
        diamondTable->setItem(row, 0, new QTableWidgetItem(o.value("type").toString()));
        diamondTable->setItem(row, 1, new QTableWidgetItem(o.value("sizeMM").toString()));
        diamondTable->setItem(row, 2, new QTableWidgetItem(o.value("quantity").toString()));
    }
}

void ModifyCatalogDialog::populateStoneTableFromJson(const QJsonArray &stoneArray)
{
    for (const QJsonValue &v : stoneArray) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        int row = stoneTable->rowCount();
        stoneTable->insertRow(row);
        stoneTable->setItem(row, 0, new QTableWidgetItem(o.value("type").toString()));
        stoneTable->setItem(row, 1, new QTableWidgetItem(o.value("sizeMM").toString()));
        stoneTable->setItem(row, 2, new QTableWidgetItem(o.value("quantity").toString()));
    }
}

void ModifyCatalogDialog::onBrowseImage()
{
    QString path = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;
    m_newSelectedImage = path;
    imagePathEdit->setText(path);
    QPixmap pm(path);
    if (!pm.isNull()) imagePreview->setPixmap(pm.scaled(imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ModifyCatalogDialog::onAddDiamondRow()
{
    int r = diamondTable->rowCount();
    diamondTable->insertRow(r);
    diamondTable->setItem(r,0,new QTableWidgetItem());
    diamondTable->setItem(r,1,new QTableWidgetItem());
    diamondTable->setItem(r,2,new QTableWidgetItem());
}
void ModifyCatalogDialog::onRemoveDiamondRow()
{
    int r = diamondTable->currentRow();
    if (r >= 0) diamondTable->removeRow(r);
}
void ModifyCatalogDialog::onAddStoneRow()
{
    int r = stoneTable->rowCount();
    stoneTable->insertRow(r);
    stoneTable->setItem(r,0,new QTableWidgetItem());
    stoneTable->setItem(r,1,new QTableWidgetItem());
    stoneTable->setItem(r,2,new QTableWidgetItem());
}
void ModifyCatalogDialog::onRemoveStoneRow()
{
    int r = stoneTable->currentRow();
    if (r >= 0) stoneTable->removeRow(r);
}

void ModifyCatalogDialog::onSaveClicked()
{
    // Validate
    QString company = companyEdit->text().trimmed();
    if (company.isEmpty()) {
        QMessageBox::warning(this, "Validation", "Company name required");
        return;
    }

    // Build gold array from goldTable (if user edited full table)
    QJsonArray goldArray;
    for (int r = 0; r < goldTable->rowCount(); ++r) {
        QJsonObject o;
        o["karat"] = goldTable->item(r, 0)->text();
        o["weight(g)"] = goldTable->item(r, 1)->text();
        goldArray.append(o);
    }

    // build diamonds
    QJsonArray diamondArray;
    for (int r = 0; r < diamondTable->rowCount(); ++r) {
        QJsonObject o;
        o["type"] = diamondTable->item(r,0) ? diamondTable->item(r,0)->text() : QString();
        o["sizeMM"] = diamondTable->item(r,1) ? diamondTable->item(r,1)->text() : QString();
        o["quantity"] = diamondTable->item(r,2) ? diamondTable->item(r,2)->text() : QString();
        diamondArray.append(o);
    }

    // stones
    QJsonArray stoneArray;
    for (int r = 0; r < stoneTable->rowCount(); ++r) {
        QJsonObject o;
        o["type"] = stoneTable->item(r,0) ? stoneTable->item(r,0)->text() : QString();
        o["sizeMM"] = stoneTable->item(r,1) ? stoneTable->item(r,1)->text() : QString();
        o["quantity"] = stoneTable->item(r,2) ? stoneTable->item(r,2)->text() : QString();
        stoneArray.append(o);
    }

    // image: if user selected a new image (filesystem path), save it using DatabaseUtils::saveImage
    QString finalImagePath = m_loadedImagePath;
    if (!m_newSelectedImage.isEmpty()) {
        QString saved = DatabaseUtils::saveImage(m_newSelectedImage);
        if (saved.isEmpty()) {
            QMessageBox::warning(this, "Image", "Failed to save image");
            return;
        }
        finalImagePath = saved;
    }

    // Call insertCatalogData with the same designNo (we're preserving designNo)
    bool okInsert = DatabaseUtils::insertCatalogData(finalImagePath,
                                                     imageTypeEdit->text(),
                                                     m_designNo,
                                                     companyEdit->text(),
                                                     goldArray,
                                                     diamondArray,
                                                     stoneArray,
                                                     noteEdit->text());
    if (!okInsert) {
        QMessageBox::critical(this, "DB", "Failed to insert updated record");
        return;
    }

    // mark old record deleted (logical delete)
    // NOTE: we set delete=1 for old active entry(ies)
    const QString connName = "ui_conn";
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (db.isValid() && db.open()) {
        QSqlQuery q(db);
        q.prepare("UPDATE image_data SET \"delete\" = 1 WHERE design_no = :dn AND \"delete\" = 0");
        q.bindValue(":dn", m_designNo);
        if (!q.exec()) {
            qWarning() << "ModifyCatalogDialog: failed to mark old record deleted:" << q.lastError().text();
        }
    }

    accept();
}


void ModifyCatalogDialog::onGoldktChanged() {
    bool ok;
    double weight24kt = goldTable->item(0,1)->text().toDouble(&ok);
    if (!ok || weight24kt <= 0) return;

    QList<int> karats = {24, 22, 20, 18, 14, 10};
    goldTable->blockSignals(true);
    for (int i = 1; i < karats.size(); ++i) {
        double newWeight = (karats[i] / 24.0) * weight24kt;
        goldTable->item(i, 1)->setText(QString::number(newWeight, 'f', 3));
    }
    goldTable->blockSignals(false);
}
