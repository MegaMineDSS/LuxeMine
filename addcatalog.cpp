#include "addcatalog.h"
#include "ui_addcatalog.h"

#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QListView>
#include <QStandardItemModel>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>

#include "databaseutils.h"
#include "utils.h"


AddCatalog::AddCatalog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Catalog)
    , jewelryMenu(new JewelryMenu(this))   // parented, auto-cleanup
{
    ui->setupUi(this);
    setWindowSize(this);

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowTitle("Add Catalog");
    setWindowIcon(QIcon(":/icon/addcatalog.png"));

    setupGoldTable();
    ui->companyName_lineEdit->setText("SHREE LAXMINARAYAN EXPORT");

    connect(ui->jewelryButton, &QPushButton::clicked, this, [this]() {
        jewelryMenu->getMenu()->popup(ui->jewelryButton->mapToGlobal(QPoint(0, ui->jewelryButton->height())));
    });

    connect(jewelryMenu, &JewelryMenu::itemSelected, this, &AddCatalog::onJewelryItemSelected);

    this->setStyleSheet(R"(
        QWidget {
            background-color: #F9FAFB;
            color: #2B2B2B;
            font-family: "Segoe UI", "Arial";
            font-size: 14px;
        }

        QLineEdit {
            background-color: #FFFFFF;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
            padding: 4px 6px;
            selection-background-color: #4A90E2;
        }

        QLineEdit:focus {
            border: 1px solid #4A90E2;
            background-color: #FDFEFF;
        }

        QPushButton {
            background-color: #E7E9EC;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
            padding: 6px 10px;
            font-weight: 500;
        }

        QPushButton:hover {
            background-color: #DDE4F2;
            border: 1px solid #4A90E2;
        }

        QPushButton:pressed {
            background-color: #C7D8F0;
            border: 1px solid #4A90E2;
        }

        QStackedWidget {
            background-color: #FFFFFF;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
        }

        QListView {
            background-color: #F9FAFB;
            border: 1px solid #C5C6C7;
            border-radius: 0px;
            color: #2B2B2B;
            outline: none;
            padding: 6px;
        }

        QListView::item {
            background-color: #FFFFFF;
            border: 1px solid #D4D4D4;
            border-radius: 0px;
            margin: 8px;
            padding: 8px 6px;
        }

        QListView::item:hover {
            background-color: #EEF3FA;
            border: 1px solid #9EB9E2;
            color: #1A1A1A;
        }

        QListView::item:selected {
            background-color: #D9E8FC;
            border: 1px solid #4A90E2;
            color: #000000;
            font-weight: 500;
        }

        QScrollBar:vertical, QScrollBar:horizontal {
            background: #F2F2F2;
            border: none;
            width: 12px;
            height: 12px;
        }

        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: #BDBDBD;
            border: 1px solid #A5A5A5;
            border-radius: 0px;
        }

        QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
            background: #9E9E9E;
        }

        QScrollBar::add-line, QScrollBar::sub-line {
            background: none;
            border: none;
            width: 0;
            height: 0;
        }
    )");



    ui->catalog_stacked->setCurrentIndex(0);

}

AddCatalog::~AddCatalog()
{
    QSqlDatabase::removeDatabase("modify_catalog_conn") ;
    delete ui;
    // jewelryMenu auto-deleted since it has "this" as parent
}

void AddCatalog::onJewelryItemSelected(const QString &item)
{
    selectedImageType = item; // e.g., "Ring (Men's Party Wear)"
    ui->jewelryButton->setText(item); // Update the button text to show the selection
}

void AddCatalog::addTableRow(QTableWidget *table, const QString &tableType)
{
    int newRow = table->rowCount();
    table->insertRow(newRow);

    // parented to table → no leaks if row is removed
    QComboBox *shapeCombo = new QComboBox(table);
    QComboBox *sizeCombo = new QComboBox(table);

    QStringList shapes = DatabaseUtils::fetchShapes(tableType);
    if (shapes.isEmpty()) {
        QMessageBox::critical(this, "Database Error", "Failed to fetch shapes for " + tableType);
        table->removeRow(newRow);
        return;
    }

    shapeCombo->addItems(shapes);

    auto populateSizeCombo = [this, sizeCombo, tableType](const QString &selectedShape) {
        sizeCombo->clear();
        QStringList sizes = DatabaseUtils::fetchSizes(tableType, selectedShape);
        if (sizes.isEmpty()) {
            QMessageBox::critical(this, "Database Error", "Failed to fetch sizes for " + selectedShape);
            return;
        }
        sizeCombo->addItems(sizes);
    };

    populateSizeCombo(shapes.first());
    connect(shapeCombo, &QComboBox::currentTextChanged, populateSizeCombo);

    table->setCellWidget(newRow, 0, shapeCombo);
    table->setCellWidget(newRow, 1, sizeCombo);
}

void AddCatalog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (ui->diaTable->hasFocus()) {
            addTableRow(ui->diaTable, "diamond");
        } else if (ui->stoneTable->hasFocus()) {
            addTableRow(ui->stoneTable, "stone");
        }
    } else if (event->key() == Qt::Key_Delete) {
        QTableWidget *focusedTable =
            ui->diaTable->hasFocus() ? ui->diaTable :
                ui->stoneTable->hasFocus() ? ui->stoneTable : nullptr;

        if (focusedTable && focusedTable->currentRow() >= 0) {
            if (QMessageBox::question(this, "Confirm Deletion", "Delete this row?") == QMessageBox::Yes) {
                int row = focusedTable->currentRow();

                // Explicitly delete widgets in the row to prevent leaks
                for (int col = 0; col < focusedTable->columnCount(); ++col) {
                    QWidget *cellWidget = focusedTable->cellWidget(row, col);
                    if (cellWidget) {
                        delete cellWidget;
                    }
                }

                focusedTable->removeRow(row);
            }
        }
    } else {
        QDialog::keyPressEvent(event);
    }
}

void AddCatalog::setupGoldTable()
{
    QList<int> karats = {24, 22, 20, 18, 14, 10};
    ui->goldTable->setRowCount(karats.size());
    ui->goldTable->setColumnCount(2); // ensure at least 2 columns

    for (int i = 0; i < karats.size(); ++i) {
        // Column 0: karat (read-only)
        QTableWidgetItem *karatItem = new QTableWidgetItem(QString::number(karats[i]) + "kt");
        karatItem->setFlags(karatItem->flags() & ~Qt::ItemIsEditable);
        ui->goldTable->setItem(i, 0, karatItem);

        // Column 1: editable weight
        ui->goldTable->setItem(i, 1, new QTableWidgetItem());
    }

    // Connect weight editing → triggers only when 24kt weight is modified
    connect(ui->goldTable, &QTableWidget::itemChanged,
            this, &AddCatalog::calculateGoldWeights);
}

// void AddCatalog::calculateGoldWeights(QTableWidgetItem *item)
// {
//     if (!item || item->column() != 1 || item->row() != 0) return; // Only recalc when 24kt weight is changed

//     bool ok;
//     double weight24kt = item->text().toDouble(&ok);
//     if (!ok || weight24kt <= 0) return;

//     QList<int> karats = {24, 22, 20, 18, 14, 10};

//     // Prevent recursive signals when updating table programmatically
//     ui->goldTable->blockSignals(true);

//     for (int i = 1; i < karats.size(); ++i) {
//         double newWeight = (karats[i] / 24.0) * weight24kt;
//         ui->goldTable->item(i, 1)->setText(QString::number(newWeight, 'f', 3));
//     }

//     ui->goldTable->blockSignals(false);
// }

void AddCatalog::calculateGoldWeights(QTableWidgetItem *item)
{
    // Only recalc when 24kt weight is changed (row 0, column 1)
    if (!item || item->column() != 1 || item->row() != 0) return;

    bool ok;
    double weight24kt = item->text().toDouble(&ok);
    if (!ok || weight24kt <= 0) return;

    QList<int> karats = {24, 22, 20, 18, 14, 10};

    if (!ui->goldTable) return; // Safety check

    // Prevent recursive signals when updating table programmatically
    ui->goldTable->blockSignals(true);

    for (int i = 1; i < karats.size(); ++i) {
        // Check that item exists before accessing
        QTableWidgetItem *w = ui->goldTable->item(i, 1);
        if (!w) {
            w = new QTableWidgetItem();
            ui->goldTable->setItem(i, 1, w); // create if missing
        }

        double newWeight = (karats[i] / 24.0) * weight24kt;
        w->setText(QString::number(newWeight, 'f', 3));
    }

    ui->goldTable->blockSignals(false);
}


void AddCatalog::on_save_insert_clicked()
{
    QString imagePath = ui->imagPath_lineEdit->text();
    QString designNo = ui->designNO_lineEdit->text();
    QString companyName = ui->companyName_lineEdit->text();
    QString note = ui->note->toPlainText();

    // if (imagePath.isEmpty() || designNo.isEmpty() || selectedImageType.isEmpty() || companyName.isEmpty()) {
    //     QMessageBox::warning(this, "Input Error", "All fields must be filled!");
    //     return;
    // }

    if (designNo.isEmpty()){
        QMessageBox::warning(this, "Input Error", "Design Number must be filled!") ;
    }

    // Build diamond JSON
    QJsonArray diamondArray;
    for (int row = 0; row < ui->diaTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *combo = qobject_cast<QComboBox*>(ui->diaTable->cellWidget(row, 0))) rowObject["type"] = combo->currentText();
        if (auto *combo = qobject_cast<QComboBox*>(ui->diaTable->cellWidget(row, 1))) rowObject["sizeMM"] = combo->currentText();
        if (auto *item = ui->diaTable->item(row, 2)) rowObject["quantity"] = item->text();
        diamondArray.append(rowObject);
    }

    // Build stone JSON
    QJsonArray stoneArray;
    for (int row = 0; row < ui->stoneTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *combo = qobject_cast<QComboBox*>(ui->stoneTable->cellWidget(row, 0))) rowObject["type"] = combo->currentText();
        if (auto *combo = qobject_cast<QComboBox*>(ui->stoneTable->cellWidget(row, 1))) rowObject["sizeMM"] = combo->currentText();
        if (auto *item = ui->stoneTable->item(row, 2)) rowObject["quantity"] = item->text();
        stoneArray.append(rowObject);
    }

    // Build gold JSON
    QJsonArray goldArray;
    for (int row = 1; row < ui->goldTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *item = ui->goldTable->item(row, 0)) rowObject["karat"] = item->text();
        if (auto *item = ui->goldTable->item(row, 1)) rowObject["weight(g)"] = item->text();
        goldArray.append(rowObject);
    }

    // Save image
    QString newImagePath = DatabaseUtils::saveImage(imagePath);
    if (newImagePath.isEmpty()) {
        QMessageBox::warning(this, "File Error", "Failed to save the image!");
        return;
    }

    // Insert DB record
    QString successReturn = DatabaseUtils::insertCatalogData(newImagePath, selectedImageType, designNo,
                                                             companyName, goldArray, diamondArray, stoneArray, note) ;
    if (successReturn == "error") {
        QMessageBox::critical(this, "Insert Error", "Failed to insert data into database!");
        return;
    }else if (successReturn == "insert") {
        QMessageBox::information(this, "Success", "Data inserted successfully!");
    } else if (successReturn == "modify") {
        QMessageBox::information(this, "Success", "Data updated successfully!");
        ui->designNO_lineEdit->setEnabled(true) ;
        ui->catalog_stacked->setCurrentIndex(2) ;
        isModifyMode = false;
    } else {
        QMessageBox::warning(this, "Error", "Error");
    }


    // Clear fields safely
    ui->imagPath_lineEdit->clear();
    ui->designNO_lineEdit->clear();
    selectedImageType.clear();
    ui->jewelryButton->setText("select jewelry type");
    ui->imageView_label_at_addImage->clear();

    // Safely clear diamond + stone tables (delete widgets first)
    auto clearTable = [](QTableWidget *table) {
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QWidget *w = table->cellWidget(row, col);
                if (w) delete w;
            }
        }
        table->setRowCount(0);
    };
    clearTable(ui->diaTable);
    clearTable(ui->stoneTable);

    // Reset gold weights
    for (int row = 0; row < ui->goldTable->rowCount(); ++row) {
        if (auto *item = ui->goldTable->item(row, 1)) item->setText("");
    }

    ui->note->clear();
}

void AddCatalog::on_brows_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");

    if (filePath.isEmpty())
        return;

    ui->imagPath_lineEdit->setText(filePath);

    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Image Error", "Failed to load the selected image.");
        return;
    }

    int labelWidth = ui->imageView_label_at_addImage->width();
    int labelHeight = ui->imageView_label_at_addImage->height();

    if (labelWidth > 0 && labelHeight > 0) {
        ui->imageView_label_at_addImage->setPixmap(
            pixmap.scaled(labelWidth, labelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void AddCatalog::on_goldTable_cellChanged(int row, int column)
{
    if (column != 1) return; // Only format weight column

    if (auto *item = ui->goldTable->item(row, column)) {
        bool ok;
        double value = item->text().toDouble(&ok);
        if (ok) {
            ui->goldTable->blockSignals(true);
            item->setText(QString::number(value, 'f', 3));
            ui->goldTable->blockSignals(false);
        }
    }
}

void AddCatalog::on_addCatalog_cancel_button_clicked()
{
    if (isModifyMode == true) {
        ui->catalog_stacked->setCurrentIndex(2) ;
        isModifyMode = false ;
        return ;
    }
    if (parentWidget() && !parentWidget()->isVisible()) {
        parentWidget()->show();
    }
    this->close();
}


void AddCatalog::on_bulk_import_button_released()
{
    QString excelPath = QFileDialog::getOpenFileName(this, "Select Excel File","","Excel Files (*.xlsx)");
    if(excelPath.isEmpty())
        return ;

    if (DatabaseUtils::excelBulkInsertCatalog(excelPath)){
        QMessageBox::information(this, "Success", "Bulk import completed successfully") ;
    } else {
        QMessageBox::critical(this, "Error", "Bulk import failed!") ;
    }
}

void AddCatalog::resetAddCatalogUI()
{
    // Clear all input fields
    ui->imagPath_lineEdit->clear();
    ui->designNO_lineEdit->clear();
    ui->designNO_lineEdit->setEnabled(true);
    ui->companyName_lineEdit->clear();
    ui->note->clear();

    // Reset jewelry button
    ui->jewelryButton->setText("Select Jewelry Type");
    selectedImageType.clear();

    // Clear image view
    ui->imageView_label_at_addImage->clear();

    // Clear tables
    ui->goldTable->setRowCount(0);
    ui->diaTable->setRowCount(0);
    ui->stoneTable->setRowCount(0);

    // Reinitialize gold table
    setupGoldTable();

    // Exit modify mode
    isModifyMode = false;
}


void AddCatalog::on_add_catalog_button_released()
{
    if (isModifyMode) {
        auto result = QMessageBox::warning(
            this,
            "Cancel Modification",
            "You are currently modifying an item.\n"
            "Do you want to cancel modification and switch to Add Catalog?",
            QMessageBox::Ok | QMessageBox::Cancel
            );

        if (result == QMessageBox::Ok) {
            resetAddCatalogUI(); // Clear everything
            ui->catalog_stacked->setCurrentIndex(0);
        } else {
            return;
        }


        // Cancel modify mode
        isModifyMode = false;

        // Re-enable designNo editing since we are switching back
        ui->designNO_lineEdit->setEnabled(true);
    }

    ui->catalog_stacked->setCurrentIndex(0);
}


void AddCatalog::loadCatalogForModify()
{
    modifyCatalogModel->clear();

    QSqlDatabase db = QSqlDatabase::database("modify_catalog_conn");

    if (!db.isOpen() && !db.open()) {
        qWarning() << "DB open failed in loadCatalogForModify:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT image_id, image_path, image_type, design_no, company_name FROM image_data")) {
        qWarning() << "Query failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        QString designNo   = query.value("design_no").toString();
        QString company    = query.value("company_name").toString();
        QString imagePath  = query.value("image_path").toString();

        QString fullPath = imagePath;
        if (!QFile::exists(fullPath)) {
            QString alt = QDir(QCoreApplication::applicationDirPath()).filePath(imagePath);
            if (QFile::exists(alt))
                fullPath = alt;
        }

        QPixmap pix;
        if (QFile::exists(fullPath))
            pix.load(fullPath);
        else
            pix.load(":/icon/no_image_1.png");

        if (pix.isNull()) {
            qWarning() << "Failed to load image:" << fullPath;
            continue;
        }

        QPixmap scaledPix = pix.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QIcon icon(scaledPix);

        QStandardItem *item = new QStandardItem(icon, designNo + "\n" + company);
        item->setEditable(false);
        item->setData(fullPath, Qt::UserRole + 1);
        item->setData(designNo, Qt::UserRole + 2);
        item->setData(company, Qt::UserRole + 3);

        modifyCatalogModel->appendRow(item);
    }

    if (modifyCatalogModel->rowCount() == 0)
        qInfo() << "No catalog data found.";
}

void AddCatalog::modifyClickedAction(const QString &designNo) {


    // Database setup
    QSqlDatabase db;
    if (QSqlDatabase::contains("modify_catalog_conn")) {
        db = QSqlDatabase::database("modify_catalog_conn");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                               .filePath("database/mega_mine_image.db"));
    }

    if (!db.open()) {
        qDebug() << "[DB ERROR] Failed to open database:" << db.lastError().text();
        return;
    }

    // Fetch record
    QSqlQuery query(db);
    query.prepare(R"(SELECT image_path, image_type, design_no, company_name, gold_weight, diamond, stone, note
                     FROM image_data
                     WHERE design_no = :design_no AND "delete" = 0)");
    query.bindValue(":design_no", designNo);

    if (!query.exec()) {
        qWarning() << "[QUERY ERROR]" << query.lastError().text();
        return;
    }

    if (!query.next()) {
        QMessageBox::warning(this, "Not Found", "No record found for this design.");
        return;
    }

    QString imagePath = QDir(QCoreApplication::applicationDirPath()).filePath(query.value("image_path").toString());
    QString imageType = query.value("image_type").toString();
    QString companyName = query.value("company_name").toString();
    QString note = query.value("note").toString();
    QString goldJson = query.value("gold_weight").toString();
    QString diamondJson = query.value("diamond").toString();
    QString stoneJson = query.value("stone").toString();


    // Fill text fields
    ui->imagPath_lineEdit->setText(imagePath);
    ui->designNO_lineEdit->setText(designNo);
    ui->companyName_lineEdit->setText(companyName);
    ui->note->setText(note);
    ui->jewelryButton->setText(imageType);ui->designNO_lineEdit->setEnabled(false) ;
    selectedImageType = imageType;

    qDebug() << "Image Path : " << imagePath ;

    // Handle image load safely
    QString absPath = imagePath;
    if (!QFile::exists(absPath))
        absPath = QDir(QCoreApplication::applicationDirPath()).filePath(imagePath);

    QPixmap pixmap;
    if (QFile::exists(absPath)) {
        pixmap.load(absPath);
        int labelWidth = ui->imageView_label_at_addImage->width();
        int labelHeight = ui->imageView_label_at_addImage->height();

        if (labelWidth > 0 && labelHeight > 0) {
            ui->imageView_label_at_addImage->setPixmap(
                pixmap.scaled(labelWidth, labelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        qDebug() << "[UI] Image loaded successfully from:" << absPath;
    } else {
        qDebug() << "[UI WARNING] Image file missing, loading placeholder.";
    }


    // Load gold table
    ui->goldTable->setRowCount(0) ;
    QJsonDocument goldDoc = QJsonDocument::fromJson(goldJson.toUtf8());
    if (goldDoc.isArray()) {
        QJsonArray arr = goldDoc.array() ;
        qDebug() << "[JSON] Parsing gold... array size:" << arr.size();

        ui->goldTable->setColumnCount(2); // ensure columns
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            int row = ui->goldTable->rowCount() ;
            ui->goldTable->insertRow(row);
            ui->goldTable->setItem(row, 0, new QTableWidgetItem(obj["karat"].toString()));
            ui->goldTable->setItem(row, 1, new QTableWidgetItem(obj["weight(g)"].toString())) ;
            qDebug() << "[GOLD] inserted row" << row << ":" << obj["karat"].toString() << "," << obj["weight(g)"].toString() ;
        }
    }

    // Load diamond table
    ui->diaTable->setRowCount(0);
    QJsonDocument diaDoc = QJsonDocument::fromJson(diamondJson.toUtf8()) ;
    if (diaDoc.isArray()) {
        QJsonArray arr = diaDoc.array();

        for (auto item : arr) {
            QJsonObject obj = item.toObject();
            int row = ui->diaTable->rowCount();
            ui->diaTable->insertRow(row);

            QComboBox *typeBox = new QComboBox(ui->diaTable);
            typeBox->addItems(DatabaseUtils::fetchShapes("diamond"));
            typeBox->setCurrentText(obj["type"].toString());

            QComboBox *sizeBox = new QComboBox(ui->diaTable);
            sizeBox->addItems(DatabaseUtils::fetchSizes("diamond", obj["type"].toString()));
            sizeBox->setCurrentText(obj["sizeMM"].toString());

            ui->diaTable->setCellWidget(row, 0, typeBox);
            ui->diaTable->setCellWidget(row, 1, sizeBox);

            ui->diaTable->setItem(row, 2, new QTableWidgetItem(obj["quantity"].toString()));
            qDebug() << "[DIAMOND] inserted row" << row << ":" << obj["type"].toString() << obj["sizeMM"].toString() << obj["quantity"].toString();
        }
    }

    // Load stone table
    ui->stoneTable->setRowCount(0);
    QJsonDocument stoneDoc = QJsonDocument::fromJson(stoneJson.toUtf8());
    if (stoneDoc.isArray()) {
        QJsonArray arr = stoneDoc.array();
        qDebug() << "[JSON] Parsing stones... array size:" << arr.size();

        for (auto item : arr) {
            QJsonObject obj = item.toObject();
            int row = ui->stoneTable->rowCount();
            ui->stoneTable->insertRow(row);

            QComboBox *typeBox = new QComboBox(ui->stoneTable);
            typeBox->addItems(DatabaseUtils::fetchShapes("stone"));
            typeBox->setCurrentText(obj["type"].toString());

            QComboBox *sizeBox = new QComboBox(ui->stoneTable);
            sizeBox->addItems(DatabaseUtils::fetchSizes("stone", obj["type"].toString()));
            sizeBox->setCurrentText(obj["sizeMM"].toString());

            ui->stoneTable->setCellWidget(row, 0, typeBox);
            ui->stoneTable->setCellWidget(row, 1, sizeBox);

            ui->stoneTable->setItem(row, 2, new QTableWidgetItem(obj["quantity"].toString()));
            qDebug() << "[STONE] inserted row" << row << ":" << obj["type"].toString() << obj["sizeMM"].toString() << obj["quantity"].toString();
        }
    }

    ui->catalog_stacked->setCurrentIndex(0);
    isModifyMode = true;
}




void AddCatalog::deleteClickedAction(const QString &designNo) {
    QMessageBox::StandardButton confirmDelete = QMessageBox::question(this, "Delete Confirmation", "Are you sure you want to delete design " + designNo + "?", QMessageBox::Yes | QMessageBox::No) ;

    if (confirmDelete == QMessageBox::Yes) {
        QSqlDatabase db;
        if (QSqlDatabase::contains("modify_catalog_conn")){
            db = QSqlDatabase::database("modify_catalog_conn");
            qDebug() << "in if " ;
        } else {
            db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
            db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                                   .filePath("database/mega_mine_image.db"));
            qDebug() << "in else " ;
        }


        if (!db.open()){
            db.open() ;
        }
        QSqlQuery query(db) ;
        query.prepare(R"(UPDATE image_data SET "delete" = 1 WHERE design_no = :design_no)") ;
        query.bindValue(":design_no", designNo) ;
        query.exec() ;

        db.close() ;

    }

}

void AddCatalog::onModifyCatalogContextMenuRightClicked(const QPoint &pos) {
    QModelIndex index = modifyCatalogView->indexAt(pos) ;
    if(!index.isValid()){
        qDebug() << "Index is not valid." << index ;
        return ;
    }

    qDebug() << "Index : " << index ;

    QString designNo = index.data(Qt::DisplayRole).toString().section('\n', 0, 0).trimmed();

    QMenu modifyRightClickMenu ;
    QAction *modifyDesignAct = modifyRightClickMenu.addAction("Modify") ;
    QAction *deleteDesignAct = modifyRightClickMenu.addAction("Delete") ;

    QAction *selectedAct = modifyRightClickMenu.exec(modifyCatalogView->viewport()->mapToGlobal(pos)) ;

    if (!selectedAct){
        return ;
    }
    if( selectedAct == modifyDesignAct) {
        AddCatalog::modifyClickedAction(designNo) ;
    } else if (selectedAct == deleteDesignAct) {
        qDebug() << "delete clicked for design. " ;
        AddCatalog::deleteClickedAction(designNo) ;
        modifyCatalogModel->removeRow(index.row()) ;

    }


}

void AddCatalog::setupModifyCatalogView()
{
    QWidget *modifyPage = ui->catalog_stacked->widget(2);
    if (!modifyPage) return;

    if (!modifyCatalogView) {
        modifyCatalogView = new QListView(modifyPage);
        modifyCatalogView->setViewMode(QListView::IconMode);
        modifyCatalogView->setIconSize(QSize(120, 120));
        modifyCatalogView->setGridSize(QSize(160, 160));
        modifyCatalogView->setResizeMode(QListView::Adjust);
        modifyCatalogView->setUniformItemSizes(true);
        modifyCatalogView->setSelectionMode(QAbstractItemView::SingleSelection);
        modifyCatalogView->setSpacing(12);
        modifyCatalogView->setItemAlignment(Qt::AlignCenter);
        modifyCatalogView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(modifyCatalogView, &QListView::customContextMenuRequested, this, &AddCatalog::onModifyCatalogContextMenuRightClicked);
        connect(modifyCatalogView, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
            qDebug() << "Connect override !" ;
            if (!index.isValid())
                return;

            QStandardItem *item = modifyCatalogModel->itemFromIndex(index);
            if (!item)
                return;

            QString designNo = item->data(Qt::UserRole).toString();
            if (designNo.isEmpty())
                return;

            if (deleteIsSet) {
                // Delete mode — do not switch stacked widget
                qDebug() << "[DoubleClick] Delete mode active for design:" << designNo;
                AddCatalog::deleteClickedAction(designNo);

                // Remove only the deleted row visually
                modifyCatalogModel->removeRow(index.row());

                // Optional: show toast or message
                QMessageBox::information(this, "Deleted", "Design " + designNo + " has been deleted.");
                return;
            }

            qDebug() << "[DoubleClick] Modify mode active for design:" << designNo;
            AddCatalog::modifyClickedAction(designNo);

        }) ;

        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(modifyPage->layout());
        if (layout) layout->addWidget(modifyCatalogView);

        modifyCatalogView->setStyleSheet(R"(
            QListView {
                background-color: #F9FAFB;
                border: 1px solid #C5C6C7;
                border-radius: 0px;
                color: #2B2B2B;
                outline: none;
                padding: 6px;
                font-size: 13px;
                font-family: "Segoe UI", "Arial";
            }

            QListView::item {
                background-color: #FFFFFF;
                border: 1px solid #D4D4D4;
                border-radius: 0px;
                margin: 8px;
                padding: 8px 6px;
            }

            QListView::item:hover {
                background-color: #EEF3FA;
                border: 1px solid #9EB9E2;
                color: #1A1A1A;
            }

            QListView::item:selected {
                background-color: #D9E8FC;
                border: 1px solid #4A90E2;
                color: #000000;
                font-weight: 500;
            }

            /* Vertical Scrollbar */
            QScrollBar:vertical {
                background: #F2F2F2;
                width: 12px;
                border: none;
                margin: 0;
            }

            QScrollBar::handle:vertical {
                background: #BDBDBD;
                border: 1px solid #A5A5A5;
                border-radius: 0px;
                min-height: 24px;
            }

            QScrollBar::handle:vertical:hover {
                background: #9E9E9E;
            }

            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                background: none;
                height: 0px;
            }

            /* Horizontal Scrollbar */
            QScrollBar:horizontal {
                background: #F2F2F2;
                height: 12px;
                border: none;
                margin: 0;
            }

            QScrollBar::handle:horizontal {
                background: #BDBDBD;
                border: 1px solid #A5A5A5;
                border-radius: 0px;
                min-width: 24px;
            }

            QScrollBar::handle:horizontal:hover {
                background: #9E9E9E;
            }

            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
                background: none;
                width: 0px;
            }

        )");

    }

    modifyCatalogView->show();

    if (!modifyCatalogModel) {
        modifyCatalogModel = new QStandardItemModel(this);
        modifyCatalogView->setModel(modifyCatalogModel);
    }

    modifyCatalogModel->clear();
}

void AddCatalog::closeEvent(QCloseEvent *event)
{
    // Closes the modify_catalog_conn when addcatalog.ui is closed.
    if (QSqlDatabase::contains("modify_catalog_conn")) {
        QSqlDatabase db = QSqlDatabase::database("modify_catalog_conn");
        if (db.isOpen()) {
            db.close();
        }
        QSqlDatabase::removeDatabase("modify_catalog_conn");
        qDebug() << "modify_catalog_conn closed and removed.";
    }

    QWidget::closeEvent(event); // or QDialog::closeEvent(event) if it's a QDialog
}


void AddCatalog::loadModifyCatalogData()
{
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    QSqlDatabase db;

    // Checking... If database is already present then no need to add that database again. It solves database duplication issue.
    if (QSqlDatabase::contains("modify_catalog_conn")) {
        db = QSqlDatabase::database("modify_catalog_conn");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "modify_catalog_conn");
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                               .filePath("database/mega_mine_image.db"));
    }
    if (!db.isOpen() && !db.open()) return;


    QSqlQuery query(db);
    if (!query.exec(R"(SELECT design_no, company_name, image_path FROM image_data WHERE "delete" = 0)")) {
        db.close();
        return;
    }

    while (query.next()) {
        QString designNo = query.value("design_no").toString();
        QString company = query.value("company_name").toString();
        QString imagePath = QDir(QCoreApplication::applicationDirPath()).filePath(query.value("image_path").toString());

        QPixmap pix = QFile::exists(imagePath)
                          ? QPixmap(imagePath)
                          : QPixmap(":/icon/no_image_1.png");

        QString companyText = company.left(25) + (company.size() > 25 ? "…" : "");
        QStandardItem *item = new QStandardItem();
        item->setIcon(QIcon(pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        item->setText(designNo + "\n" + companyText);
        item->setTextAlignment(Qt::AlignCenter);
        item->setEditable(false);
        item->setSizeHint(QSize(160, 160));
        item->setData(designNo, Qt::UserRole) ;

        modifyCatalogModel->appendRow(item);
    }

    db.close();
}


void AddCatalog::on_modify_catalog_button_released()
{
    AddCatalog::deleteIsSet = false ;
    ui->catalog_stacked->setCurrentIndex(2);
    QWidget *modifyPage = ui->catalog_stacked->widget(2);
    if (!modifyPage) return;

    // Set professional white background
    // modifyPage->setStyleSheet("background-color: #F8F8F8; border: 1.5px solid #999999;  border-radius:1px;");

    // Ensure there is a layout
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(modifyPage->layout());
    if (!layout) {
        layout = new QVBoxLayout(modifyPage);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(6);
        modifyPage->setLayout(layout);
    }

    static QLineEdit *modifySearchBar = nullptr ;
    if(!modifySearchBar) {
        modifySearchBar = new QLineEdit(modifyPage) ;
        modifySearchBar->setPlaceholderText("Search by design number...") ;
        modifySearchBar->setClearButtonEnabled(true) ;
        modifySearchBar->setFixedHeight(32) ;
        // modifySearchBar->setStyleSheet(R"(
        //     QLineEdit { border: 1px solid #999999;  border-radius: 6px; padding-left: 8px;  background-color: #FFFFFF;  font-size: 14px;    }
        //     QLineEdit:focus {  border: 1px solid #0078D7;   background-color: #F9F9F9;  }
        // )") ;
    }

    layout->insertWidget(0, modifySearchBar) ;

    connect(modifySearchBar, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!modifyCatalogModel) return;
        if (text.trimmed().isEmpty()) {
            for (int i = 0; i < modifyCatalogModel->rowCount(); ++i){
                modifyCatalogView->setRowHidden(i, false);
            }
            return;
        }
        for (int i = 0; i < modifyCatalogModel->rowCount(); ++i) {
            QStandardItem *item = modifyCatalogModel->item(i);
            const QString designNo = item->data(Qt::UserRole).toString();
            const bool match = designNo.contains(text, Qt::CaseInsensitive);
            modifyCatalogView->setRowHidden(i, !match);
        }

    });


    setupModifyCatalogView();
    loadModifyCatalogData();
}


void AddCatalog::on_delete_catalog_button_released()
{
    AddCatalog::deleteIsSet = true ;
    ui->catalog_stacked->setCurrentIndex(2);
}

