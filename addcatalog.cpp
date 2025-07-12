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

#include "DatabaseUtils.h"
#include "Utils.h"


AddCatalog::AddCatalog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddCatalog)
    , jewelryMenu(new JewelryMenu(this))
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    ui->setupUi(this);
    setWindowSize(this);
    // jewelryMenu = new JewelryMenu(this);

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowTitle("Add Catalog");  // Set the window title
    setWindowIcon(QIcon(":/icon/addcatalog.png"));  // Set the window icon


    setupGoldTable();
    ui->companyName_lineEdit->setText("SHREE LAXMINARAYAN EXPORT");

    connect(ui->jewelryButton, &QPushButton::clicked, this, [this]() {
        jewelryMenu->getMenu()->popup(ui->jewelryButton->mapToGlobal(QPoint(0, ui->jewelryButton->height())));
    });

    // Connect the JewelryMenu signal to our slot
    connect(jewelryMenu, &JewelryMenu::itemSelected, this, &AddCatalog::onJewelryItemSelected);

}

AddCatalog::~AddCatalog()
{
    delete ui;
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

    QComboBox *shapeCombo = new QComboBox();
    QComboBox *sizeCombo = new QComboBox();

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
        QTableWidget *focusedTable = ui->diaTable->hasFocus() ? ui->diaTable : ui->stoneTable->hasFocus() ? ui->stoneTable : nullptr;
        if (focusedTable && focusedTable->currentRow() >= 0) {
            if (QMessageBox::question(this, "Confirm Deletion", "Delete this row?") == QMessageBox::Yes) {
                focusedTable->removeRow(focusedTable->currentRow());
            }
        }
    } else {
        QDialog::keyPressEvent(event);
    }
}


void AddCatalog::setupGoldTable() {

    ui->goldTable->setRowCount(6);

    // Define karat values
    QList<int> karats = {24, 22, 20, 18, 14, 10};

    for (int i = 0; i < karats.size(); ++i) {
        // Set non-editable karat values
        QTableWidgetItem *karatItem = new QTableWidgetItem(QString::number(karats[i]) + "kt");
        karatItem->setFlags(karatItem->flags() & ~Qt::ItemIsEditable);
        ui->goldTable->setItem(i, 0, karatItem);

        // Set editable weight field
        ui->goldTable->setItem(i, 1, new QTableWidgetItem());
    }

    // Connect weight editing to calculation function
    connect(ui->goldTable, &QTableWidget::itemChanged, this, &AddCatalog::calculateGoldWeights);
}

void AddCatalog::calculateGoldWeights(QTableWidgetItem *item) {
    if (!item || item->column() != 1 || item->row() != 0) return; // Only update when 24kt is changed

    bool ok;
    double weight24kt = item->text().toDouble(&ok);
    if (!ok || weight24kt <= 0) return; // Ignore invalid input

    QList<int> karats = {24, 22, 20, 18, 14, 10};
    for (int i = 1; i < karats.size(); ++i) {
        double newWeight = (karats[i] / 24.0) * weight24kt;
        ui->goldTable->item(i, 1)->setText(QString::number(newWeight, 'f', 3));
    }
}

void AddCatalog::on_save_insert_clicked()
{
    QString imagePath = ui->imagPath_lineEdit->text();
    QString designNo = ui->designNO_lineEdit->text();
    QString companyName = ui->companyName_lineEdit->text();
    QString note = ui->note->toPlainText();

    if (imagePath.isEmpty() || designNo.isEmpty() || selectedImageType.isEmpty() || companyName.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "All fields must be filled!");
        return;
    }

    QJsonArray diamondArray;
    for (int row = 0; row < ui->diaTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *combo = qobject_cast<QComboBox*>(ui->diaTable->cellWidget(row, 0))) rowObject["type"] = combo->currentText();
        if (auto *combo = qobject_cast<QComboBox*>(ui->diaTable->cellWidget(row, 1))) rowObject["sizeMM"] = combo->currentText();
        if (auto *item = ui->diaTable->item(row, 2)) rowObject["quantity"] = item->text();
        diamondArray.append(rowObject);
    }

    QJsonArray stoneArray;
    for (int row = 0; row < ui->stoneTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *combo = qobject_cast<QComboBox*>(ui->stoneTable->cellWidget(row, 0))) rowObject["type"] = combo->currentText();
        if (auto *combo = qobject_cast<QComboBox*>(ui->stoneTable->cellWidget(row, 1))) rowObject["sizeMM"] = combo->currentText();
        if (auto *item = ui->stoneTable->item(row, 2)) rowObject["quantity"] = item->text();
        stoneArray.append(rowObject);
    }

    QJsonArray goldArray;
    for (int row = 1; row < ui->goldTable->rowCount(); ++row) {
        QJsonObject rowObject;
        if (auto *item = ui->goldTable->item(row, 0)) rowObject["karat"] = item->text();
        if (auto *item = ui->goldTable->item(row, 1)) rowObject["weight(g)"] = item->text();
        goldArray.append(rowObject);
    }

    QString newImagePath = DatabaseUtils::saveImage(imagePath);
    if (newImagePath.isEmpty()) {
        QMessageBox::warning(this, "File Error", "Failed to save the image!");
        return;
    }

    if (!DatabaseUtils::insertCatalogData(newImagePath, selectedImageType, designNo, companyName, goldArray, diamondArray, stoneArray, note)) {
        QMessageBox::critical(this, "Insert Error", "Failed to insert data into database!");
        return;
    }

    QMessageBox::information(this, "Success", "Data inserted successfully!");

    ui->imagPath_lineEdit->clear();
    ui->designNO_lineEdit->clear();
    selectedImageType.clear();
    ui->jewelryButton->setText("select jewelry type");
    ui->imageView_label_at_addImage->clear();
    ui->diaTable->setRowCount(0);
    for (int row = 0; row < ui->goldTable->rowCount(); ++row) {
        if (auto *item = ui->goldTable->item(row, 1)) item->setText("");
    }
    ui->stoneTable->setRowCount(0);
    ui->note->clear();
}

void AddCatalog::on_brows_clicked()
{
    // Open file dialog to select an image
    QString filePath = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (filePath.isEmpty()) return;

    ui->imagPath_lineEdit->setText(filePath);
    QPixmap pixmap(filePath);
    int labelWidth = ui->imageView_label_at_addImage->width();
    int labelHeight = ui->imageView_label_at_addImage->height();
    ui->imageView_label_at_addImage->setPixmap(pixmap.scaled(labelWidth, labelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AddCatalog::on_goldTable_cellChanged(int row, int column)
{
    if (auto *item = ui->goldTable->item(row, column)) {
        bool ok;
        double value = item->text().toDouble(&ok);
        if (ok) item->setText(QString::number(value, 'f', 3));
    }
}



void AddCatalog::on_addCatalog_cancel_button_clicked()
{
    if (parentWidget()) {
        parentWidget()->show();
    }

    this->close();  // Close after showing parent
}

