#include "ordermenu.h"
#include "ui_ordermenu.h"

#include <QMap>
#include <QStringList>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFontMetrics>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFileDialog>
#include <QDir>


OrderMenu::OrderMenu(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderMenu)
{
    ui->setupUi(this);
    setFixedSize(this->size());
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    setupMetalComboBoxes();
    setupCertificateComboBoxes();
    setupDateFields();
    setupPolishOptions();
    setupImageUploadHandlers();
}

void OrderMenu::setupMetalComboBoxes() {
    ui->metalNameComboBox->addItem("-");
    ui->metalNameComboBox->addItems({"Gold", "Silver", "Platinum", "Copper", "Palladium", "Titanium", "Tungsten", "Brass"});

    QMap<QString, QStringList> purityMap;
    purityMap["Gold"] = { "24K (99.9%)", "22K (91.6%)", "20K (83.3%)", "18K (75%)", "14K (58.5%)", "10K (41.7%)", "9K (37.5%)" };
    purityMap["Silver"] = { "Fine Silver (99.9%)", "Sterling Silver (92.5%)", "Coin Silver (90%)" };
    purityMap["Platinum"] = { "950 Platinum", "900 Platinum", "850 Platinum" };
    purityMap["Copper"] = { "Pure Copper (99.9%)" };
    purityMap["Palladium"] = { "950 Palladium", "900 Palladium" };
    purityMap["Titanium"] = { "Commercial Pure", "Grade 5 (90% Ti)" };
    purityMap["Tungsten"] = { "Tungsten Carbide" };
    purityMap["Brass"] = { "Red Brass (85% Cu)", "Yellow Brass (65% Cu)", "Cartridge Brass (70% Cu)" };

    ui->metalColorComboBox->addItems({"-", "Yellow", "White", "Rose", "Green", "Champagne", "Black (Rhodium)", "Two-Tone"});
    ui->metalColorComboBox->setEnabled(false);

    connect(ui->metalNameComboBox, &QComboBox::currentTextChanged, this, [=](const QString &metal) {
        ui->metalPurityComboBox->clear();
        if (purityMap.contains(metal)) {
            ui->metalPurityComboBox->addItem("-");
            ui->metalPurityComboBox->addItems(purityMap.value(metal));
        } else {
            ui->metalPurityComboBox->addItem("-");
        }

        bool enableColor = (metal == "Gold");
        ui->metalColorComboBox->setEnabled(enableColor);
        ui->metalColorComboBox->setCurrentIndex(0);
    });

    ui->metalNameComboBox->setCurrentIndex(0);
    ui->metalPurityComboBox->addItem("-");
}

void OrderMenu::setupCertificateComboBoxes() {
    QStringList metalCertificates = {
        "Hallmark", "Purity Card", "Purity Mark", "Assay Certificate", "NABL Certificate",
        "ISO Certification", "In-house Certificate", "Recycled Metal Certificate"
    };

    QStandardItemModel* metalModel = new QStandardItemModel(ui->metalCertiTypeComboBox);
    for (const QString& cert : metalCertificates) {
        QStandardItem* item = new QStandardItem(cert);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        metalModel->appendRow(item);
    }
    ui->metalCertiTypeComboBox->setModel(metalModel);
    ui->metalCertiTypeComboBox->setEditable(true);
    ui->metalCertiTypeComboBox->lineEdit()->setReadOnly(true);
    connect(metalModel, &QStandardItemModel::itemChanged, this, [=]() {
        QStringList selected;
        for (int i = 0; i < metalModel->rowCount(); ++i) {
            QStandardItem* item = metalModel->item(i);
            if (item->checkState() == Qt::Checked) selected << item->text();
        }
        ui->metalCertiTypeComboBox->lineEdit()->setText(selected.join(", "));
    });

    ui->metalCertiNameComboBox->addItem("-");
    ui->metalCertiNameComboBox->addItems({"Gold", "Silver", "Platinum", "Copper", "Palladium", "Titanium"});
    connect(ui->metalCertiNameComboBox, &QComboBox::currentTextChanged, this, [=]() {
        for (int i = 0; i < metalModel->rowCount(); ++i)
            metalModel->item(i)->setCheckState(Qt::Unchecked);
        ui->metalCertiTypeComboBox->lineEdit()->clear();
    });

    ui->diaCertiNameComboBox->addItem("-");
    ui->diaCertiNameComboBox->addItems({"Diamond", "CVD", "HPHT", "Stone"});

    QStringList diaCertTypes = {"-", "IGI", "GIA", "HRD", "SGL"};
    QStandardItemModel* diaCertModel = new QStandardItemModel(ui->diaCertiTypeComboBox);
    for (const QString& cert : diaCertTypes) {
        QStandardItem* item = new QStandardItem(cert);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        diaCertModel->appendRow(item);
    }
    ui->diaCertiTypeComboBox->setModel(diaCertModel);
    ui->diaCertiTypeComboBox->setEditable(true);
    ui->diaCertiTypeComboBox->lineEdit()->setReadOnly(true);
    connect(diaCertModel, &QStandardItemModel::itemChanged, this, [=]() {
        QStringList selected;
        for (int i = 1; i < diaCertModel->rowCount(); ++i) {
            if (diaCertModel->item(i)->checkState() == Qt::Checked)
                selected << diaCertModel->item(i)->text();
        }
        ui->diaCertiTypeComboBox->lineEdit()->setText(selected.join(", "));
    });
}

void OrderMenu::setupDateFields() {
    QDate today = QDate::currentDate();
    QDate minDeliveryDate = today.addDays(1);

    ui->orderDateDateEdit->setDate(today);
    ui->orderDateDateEdit->setCalendarPopup(true);
    ui->orderDateDateEdit->setReadOnly(true);
    ui->orderDateDateEdit->setEnabled(false);

    ui->deliveryDateDateEdit->setDate(minDeliveryDate);
    ui->deliveryDateDateEdit->setMinimumDate(minDeliveryDate);
    ui->deliveryDateDateEdit->setCalendarPopup(true);
}

void OrderMenu::setupPolishOptions() {
    ui->pesSakiComboBox->addItem("-");
    ui->pesSakiComboBox->addItems({"South Pes", "Paip South", "Regular", "Antic Pes"});

    ui->chainLockComboBox->addItem("-");
    ui->chainLockComboBox->addItems({"S Huck", "Italian Looks", "Kadi Akda"});

    ui->polishComboBox->addItem("-");
    ui->polishComboBox->addItems({"Regular", "Export", "Dal", "Mat", "Raf"});

    ui->settingLabourComboBox->addItem("-");
    ui->settingLabourComboBox->addItems({"OpenSetting-150", "CloseSetting-145", "Kundan-120", "Stone-100"});

    ui->metalStempComboBox->addItem("-");
    ui->metalStempComboBox->addItems({"Company Name Mark", "Logo Mark", "PurityMark + DiamondMark"});

    ui->payMethodComboBox->addItem("-");
    ui->payMethodComboBox->addItems({
        "Cash", "RTGS", "Author Bank Pay", "Gold convert pay",
        "Diamond convert pay", "Hawala Pay", "Angdiya", "VPP", "UPI", "Card Pay"
    });
}

void OrderMenu::setupImageUploadHandlers() {
    connect(ui->imageLabel1, &ImageClickLabel::rightClicked, this, [=]() {
        QString path = selectAndSaveImage("image1");
        if (!path.isEmpty()) {
            ui->imageLabel1->setPixmap(QPixmap(path).scaled(ui->imageLabel1->size(), Qt::KeepAspectRatio));
            imagePath1 = path;
        }
    });

    connect(ui->imageLabel2, &ImageClickLabel::rightClicked, this, [=]() {
        QString path = selectAndSaveImage("image2");
        if (!path.isEmpty()) {
            ui->imageLabel2->setPixmap(QPixmap(path).scaled(ui->imageLabel2->size(), Qt::KeepAspectRatio));
            imagePath2 = path;
        }
    });
}

OrderMenu::~OrderMenu()
{
    delete ui;
}

void OrderMenu::setInitialInfo(const QString &sellerName, const QString &sellerId,
                               const QString &partyName, const QString &partyId, const QString &partyAddress,
                               const QString &partyCity, const QString &partyState,
                               const QString &partyCountry)
{
    // Set seller info
    ui->sellerNameLineEdit->setText(sellerName);
    ui->sellerIdLineEdit->setText(sellerId);
    ui->sellerNameLineEdit->setReadOnly(true);
    ui->sellerIdLineEdit->setReadOnly(true);
    currentSellerId = sellerId;
    currentSellerName = sellerName;

    // Set party info
    ui->partyNameLineEdit->setText(partyName);
    ui->addressLineEdit->setText(partyAddress);
    ui->cityLineEdit->setText(partyCity);
    ui->stateLineEdit->setText(partyState);
    ui->countryLineEdit->setText(partyCountry);

    currentPartyName = partyName;
    currentPartyAddress = partyAddress;
    currentPartyCity = partyCity;
    currentPartyState = partyState;
    currentPartyCountry = partyCountry;
    currentPartyId = partyId;

}

void OrderMenu::insertDummyOrder()
{
    // Step 1: Insert and get ID
    dummyOrderId = DatabaseUtils::insertDummyOrder("Test Seller", "SELL123", "TEMP_PARTY");
    if (dummyOrderId == -1) {
        QMessageBox::critical(this, "Insert Failed", "Could not insert dummy order.");
        return;
    }

    // Step 2: Generate values
    int jobNum = DatabaseUtils::getNextJobNumber();
    QString finalJobNo = QString("JOB%1").arg(jobNum, 5, 10, QChar('0'));
    int orderNum = DatabaseUtils::getNextOrderNumberForSeller(currentSellerId);
    QString finalOrderNo = QString("%1%2").arg(currentSellerId).arg(orderNum, 5, 10, QChar('0'));

    // Step 3: Update dummy row
    if (!DatabaseUtils::updateDummyOrder(dummyOrderId, finalJobNo, finalOrderNo)) {
        QMessageBox::critical(this, "Update Failed", "Could not update dummy order numbers.");
        return;
    }

    // Step 4: Update UI
    ui->jobNoLineEdit->setText(finalJobNo);
    ui->orderNoLineEdit->setText(finalOrderNo);
    ui->jobNoLineEdit->setReadOnly(true);
    ui->orderNoLineEdit->setReadOnly(true);

    qDebug() << "Dummy order created with ID:" << dummyOrderId
             << ", JobNo:" << finalJobNo << ", OrderNo:" << finalOrderNo;
}

QString OrderMenu::selectAndSaveImage(const QString &prefix) {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Image", QDir::homePath(), "Images (*.png *.jpg *.jpeg)");
    if (filePath.isEmpty()) return "";

    QDir appDir(QCoreApplication::applicationDirPath());
    QString imageDirPath = appDir.filePath("images");

    QDir().mkpath(imageDirPath);  // ensure images/ exists

    QString fileName = prefix + "_" + QFileInfo(filePath).fileName();
    QString destPath = QDir(imageDirPath).filePath(fileName);

    if (QFile::copy(filePath, destPath)) {
        return destPath;  // success
    } else {
        QMessageBox::warning(this, "Image Copy Failed", "Could not copy image.");
        return "";
    }
}

void OrderMenu::closeEvent(QCloseEvent *event)
{
    if (!isSaved && dummyOrderId != -1) {
        QDir::setCurrent(QCoreApplication::applicationDirPath());
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "new_conn");


        db.setDatabaseName("database/mega_mine_orderbook.db");
        if (!db.open()) {
            qDebug() << "Error:" << db.lastError().text();
            return;
        }

        QSqlQuery cleanup(db);  // Associate with the "new_conn" connection
        cleanup.prepare("DELETE FROM \"OrderBook-Detail\" WHERE id = :id AND isSaved = 0");
        cleanup.bindValue(":id", dummyOrderId);
        // qDebug()<<dummyOrderId;
        cleanup.exec();

        db.close();


    }
    QSqlDatabase::removeDatabase("new_conn");
    QDialog::closeEvent(event);
}

void OrderMenu::on_savePushButton_clicked()
{
    OrderData order;
    order.id = dummyOrderId;
    order.jobNo = ui->jobNoLineEdit->text();
    order.orderNo = ui->orderNoLineEdit->text();

    // Fetch all remaining fields
    order.sellerName = ui->sellerNameLineEdit->text().trimmed();
    order.sellerId = ui->sellerIdLineEdit->text().trimmed();
    order.orderDate = ui->orderDateDateEdit->date().toString("yyyy-MM-dd");
    order.deliveryDate = ui->deliveryDateDateEdit->date().toString("yyyy-MM-dd");

    order.partyId = currentPartyId;
    // qDebug()<<order.partyId<<"------oid";
    order.partyName = ui->partyNameLineEdit->text().trimmed();
    order.clientId = ui->clientIdLineEdit->text().trimmed();
    order.agencyId = ui->agencyIdLineEdit->text().trimmed();
    order.shopId = ui->shopIdLineEdit->text().trimmed();
    order.retailleId = ui->reteailleIdLineEdit->text().trimmed();
    order.starId = ui->starIdLineEdit->text().trimmed();
    order.address = ui->addressLineEdit->text();
    order.city = ui->cityLineEdit->text().trimmed();
    order.state = ui->stateLineEdit->text().trimmed();
    order.country = ui->countryLineEdit->text().trimmed();

    order.productName = ui->productNameLineEdit->text().trimmed();
    order.productPis = ui->productPicLineEdit->text();
    order.approxProductWt = ui->approxProductWtLineEdit->text();
    order.metalPrice = ui->metalPriceLineEdit->text();

    order.metalName = ui->metalNameComboBox->currentText();
    order.metalPurity = ui->metalPurityComboBox->currentText();
    order.metalColor = ui->metalColorComboBox->currentText();

    order.sizeNo = ui->sizeNoLineEdit->text();
    order.sizeMM = ui->sizeMMLineEdit->text();
    order.length = ui->lengthLineEdit->text();
    order.width = ui->widthLineEdit->text();
    order.height = ui->heightLineEdit->text();

    order.diaPacific = ui->diaPacificLineEdit->text().trimmed();
    order.diaPurity = ui->diaPurityLineEdit->text().trimmed();
    order.diaColor = ui->diaColorLineEdit->text().trimmed();
    order.diaPrice = ui->diaPriceLineEdit->text();

    order.stPacific = ui->stPacificLineEdit->text().trimmed();
    order.stPurity = ui->stPurityLineEdit->text().trimmed();
    order.stColor = ui->stColorLineEdit->text().trimmed();
    order.stPrice = ui->stPriceLineEdit->text();

    order.designNo1 = ui->designNoLineEdit1->text().trimmed();
    order.designNo2 = ui->designNoLineEdit2->text().trimmed();

    order.image1Path = imagePath1;
    order.image2Path = imagePath2;

    order.metalCertiName = ui->metalCertiNameComboBox->currentText();
    QStringList metalCertList;
    QStandardItemModel* metalModel = qobject_cast<QStandardItemModel*>(ui->metalCertiTypeComboBox->model());
    for (int i = 0; i < metalModel->rowCount(); ++i) {
        QStandardItem* item = metalModel->item(i);
        if (item->checkState() == Qt::Checked) {
            metalCertList << item->text();
        }
    }
    order.metalCertiType = metalCertList.join(", ");

    order.diaCertiName = ui->diaCertiNameComboBox->currentText();
    QStringList diaCertList;
    QStandardItemModel* diaModel = qobject_cast<QStandardItemModel*>(ui->diaCertiTypeComboBox->model());
    for (int i = 0; i < diaModel->rowCount(); ++i) {
        QStandardItem* item = diaModel->item(i);
        if (item->checkState() == Qt::Checked) {
            diaCertList << item->text();
        }
    }
    order.diaCertiType = diaCertList.join(", ");

    order.pesSaki = ui->pesSakiComboBox->currentText();
    order.chainLock = ui->chainLockComboBox->currentText();
    order.polish = ui->polishComboBox->currentText();

    order.settingLabour = ui->settingLabourComboBox->currentText();
    order.metalStemp = ui->metalStempComboBox->currentText();
    order.paymentMethod = ui->payMethodComboBox->currentText();

    order.totalAmount = ui->totalAmountLineEdit->text();
    order.advance = ui->advanceLineEdit->text();
    order.remaining = ui->remainingLineEdit->text();

    order.note = ui->noteTextEdit->toPlainText().trimmed();
    order.extraDetail = ui->extraNoteTextEdit->toPlainText().trimmed();


    if (DatabaseUtils::saveOrder(order)) {
        QMessageBox::information(this, "Success", "Order saved successfully.");
        isSaved = true;
    } else {
        QMessageBox::critical(this, "Update Failed", "Order update failed. Check console.");
    }
}
