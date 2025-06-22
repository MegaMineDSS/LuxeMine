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
#include<QDebug>



OrderMenu::OrderMenu(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderMenu)
{
    ui->setupUi(this);
    setFixedSize(this->size());  // lock window size
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    auto adjustComboBoxPopupWidth = [](QComboBox *comboBox) {
        int maxWidth = 0;
        QFontMetrics fm(comboBox->font());
        for (int i = 0; i < comboBox->count(); ++i) {
            int itemWidth = fm.horizontalAdvance(comboBox->itemText(i));
            if (itemWidth > maxWidth) maxWidth = itemWidth;
        }
        comboBox->view()->setMinimumWidth(maxWidth + 30); // 30 for padding/scrollbar
    };

    // 1. Set metal name options
    ui->metalNameComboBox->addItem("-");
    ui->metalNameComboBox->addItems({"Gold", "Silver", "Platinum", "Copper", "Palladium", "Titanium", "Tungsten", "Brass"});

    // 2. Map purities by metal
    QMap<QString, QStringList> purityMap;
    purityMap["Gold"] = { "24K (99.9%)", "22K (91.6%)", "20K (83.3%)", "18K (75%)", "14K (58.5%)", "10K (41.7%)", "9K (37.5%)" };
    purityMap["Silver"] = { "Fine Silver (99.9%)", "Sterling Silver (92.5%)", "Coin Silver (90%)" };
    purityMap["Platinum"] = { "950 Platinum", "900 Platinum", "850 Platinum" };
    purityMap["Copper"] = { "Pure Copper (99.9%)" };
    purityMap["Palladium"] = { "950 Palladium", "900 Palladium" };
    purityMap["Titanium"] = { "Commercial Pure", "Grade 5 (90% Ti)" };
    purityMap["Tungsten"] = { "Tungsten Carbide" };
    purityMap["Brass"] = { "Red Brass (85% Cu)", "Yellow Brass (65% Cu)", "Cartridge Brass (70% Cu)" };

    // 3. Metal Colors
    QStringList metalColors = { "-", "Yellow", "White", "Rose", "Green", "Champagne", "Black (Rhodium)", "Two-Tone" };
    ui->metalColorComboBox->addItems(metalColors);
    ui->metalColorComboBox->setEnabled(false);  // Disabled by default

    // 4. Connect metalName to update metalPurity and enable/disable metalColor
    connect(ui->metalNameComboBox, &QComboBox::currentTextChanged, this, [=](const QString &metal) {
        ui->metalPurityComboBox->clear();
        if (purityMap.contains(metal)) {
            ui->metalPurityComboBox->addItem("-");
            ui->metalPurityComboBox->addItems(purityMap.value(metal));
        } else {
            ui->metalPurityComboBox->addItem("-");
        }

        // Enable metal color only for Gold
        bool enableColor = (metal == "Gold");
        ui->metalColorComboBox->setEnabled(enableColor);
        ui->metalColorComboBox->setCurrentIndex(0); // Reset to "-"
    });

    // 5. Set default selection
    ui->metalNameComboBox->setCurrentIndex(0);
    ui->metalPurityComboBox->addItem("-");


    //dateEdit fixed
    // Get today's date
    QDate today = QDate::currentDate();
    QDate minDeliveryDate = today.addDays(1);

    // Set Order Date to today and disable editing
    ui->orderDateDateEdit->setDate(today);
    ui->orderDateDateEdit->setCalendarPopup(true); // Optional: calendar popup
    ui->orderDateDateEdit->setReadOnly(true);
    ui->orderDateDateEdit->setEnabled(false);      // Completely disables selection

    // Set Delivery Date: minimum date = tomorrow
    ui->deliveryDateDateEdit->setDate(minDeliveryDate);
    ui->deliveryDateDateEdit->setMinimumDate(minDeliveryDate);
    ui->deliveryDateDateEdit->setCalendarPopup(true); // optional

    //certificate combo box
    // Create list of metal certificate options
    QStringList metalCertificates = {
        "Hallmark",
        "Purity Card",
        "Purity Mark",
        "Assay Certificate",
        "NABL Certificate",
        "ISO Certification",
        "In-house Certificate",
        "Recycled Metal Certificate"
    };

    // Access the model of the ComboBox
    QStandardItemModel* model = new QStandardItemModel(ui->metalCertiTypeComboBox);

    // Add each certificate as a checkable item
    for (const QString& cert : metalCertificates) {
        QStandardItem* item = new QStandardItem(cert);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        model->appendRow(item);
    }

    // Set the model on the ComboBox
    ui->metalCertiTypeComboBox->setModel(model);

    // Optional: Make ComboBox editable to display selected items
    ui->metalCertiTypeComboBox->setEditable(true);
    ui->metalCertiTypeComboBox->lineEdit()->setReadOnly(true);

    // Function to update display text based on selected items
    connect(model, &QStandardItemModel::itemChanged, this, [=]() {
        QStringList selected;
        for (int i = 0; i < model->rowCount(); ++i) {
            QStandardItem* item = model->item(i);
            if (item->checkState() == Qt::Checked) {
                selected << item->text();
            }
        }
        ui->metalCertiTypeComboBox->lineEdit()->setText(selected.join(", "));
    });

    // 1. Set available metal options for certificates
    ui->metalCertiNameComboBox->addItem("-");
    ui->metalCertiNameComboBox->addItems({
        "Gold",
        "Silver",
        "Platinum",
        "Copper",
        "Palladium",
        "Titanium"
    });

    // 2. Optional: Auto-clear certificate selection on metal change
    connect(ui->metalCertiNameComboBox, &QComboBox::currentTextChanged, this, [=](const QString &metal) {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->metalCertiTypeComboBox->model());
        for (int i = 0; i < model->rowCount(); ++i) {
            QStandardItem* item = model->item(i);
            item->setCheckState(Qt::Unchecked);
        }
        ui->metalCertiTypeComboBox->lineEdit()->clear();
    });

    // Set options for diamond/stone certificate material
    ui->diaCertiNameComboBox->addItem("-");
    ui->diaCertiNameComboBox->addItems({
        "Diamond",
        "CVD",
        "HPHT",
        "Stone"
    });

    // Set options for diamond/stone certificate authorities
    // 1. List of available diamond/stone certification types
    QStringList diaCertTypes = {
        "-",
        "IGI",
        "GIA",
        "HRD",
        "SGL"
    };

    // 2. Set up model for checkbox-enabled combo box
    QStandardItemModel* diaCertModel = new QStandardItemModel(ui->diaCertiTypeComboBox);

    for (const QString& cert : diaCertTypes) {
        QStandardItem* item = new QStandardItem(cert);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        diaCertModel->appendRow(item);
    }

    // 3. Apply model and make combo editable to show selected items
    ui->diaCertiTypeComboBox->setModel(diaCertModel);
    ui->diaCertiTypeComboBox->setEditable(true);
    ui->diaCertiTypeComboBox->lineEdit()->setReadOnly(true);

    // 4. Update line edit text with selected items
    connect(diaCertModel, &QStandardItemModel::itemChanged, this, [=]() {
        QStringList selected;
        for (int i = 1; i < diaCertModel->rowCount(); ++i) {
            QStandardItem* item = diaCertModel->item(i);
            if (item->checkState() == Qt::Checked) {
                selected << item->text();
            }
        }
        ui->diaCertiTypeComboBox->lineEdit()->setText(selected.join(", "));
    });

    // Set up pesSakiComboBox
    ui->pesSakiComboBox->addItem("-");
    ui->pesSakiComboBox->addItems({
        "South Pes",
        "Paip South",
        "Regular",
        "Antic Pes"
    });

    // Set up chainLockComboBox
    ui->chainLockComboBox->addItem("-");
    ui->chainLockComboBox->addItems({
        "S Huck",
        "Italian Looks",
        "Kadi Akda"
    });

    // Set up polishComboBox
    ui->polishComboBox->addItem("-");
    ui->polishComboBox->addItems({
        "Regular",
        "Export",
        "Dal",
        "Mat",
        "Raf"
    });

    // Setting Labour ComboBox
    ui->settingLabourComboBox->addItem("-");
    ui->settingLabourComboBox->addItems({
        "OpenSetting-150",
        "CloseSetting-145",
        "Kundan-120",
        "Stone-100"
    });

    // Metal Stamp ComboBox
    ui->metalStempComboBox->addItem("-");
    ui->metalStempComboBox->addItems({
        "Company Name Mark",
        "Logo Mark",
        "PurityMark + DiamondMark"
    });

    ui->payMethodComboBox->addItem("-");
    ui->payMethodComboBox->addItems({
        "Cash",
        "RTGS",
        "Author Bank Pay",
        "Gold convert pay",
        "Diamond convert pay",
        "Hawala Pay",
        "Angdiya",
        "VPP",
        "UPI",
        "Card Pay"
    });

    adjustComboBoxPopupWidth(ui->payMethodComboBox);
    adjustComboBoxPopupWidth(ui->diaCertiTypeComboBox);
    adjustComboBoxPopupWidth(ui->diaCertiNameComboBox);
    adjustComboBoxPopupWidth(ui->metalCertiNameComboBox);
    adjustComboBoxPopupWidth(ui->metalCertiTypeComboBox);
    adjustComboBoxPopupWidth(ui->metalStempComboBox);
    adjustComboBoxPopupWidth(ui->settingLabourComboBox);
    adjustComboBoxPopupWidth(ui->polishComboBox);
    adjustComboBoxPopupWidth(ui->chainLockComboBox);
    adjustComboBoxPopupWidth(ui->pesSakiComboBox);
    adjustComboBoxPopupWidth(ui->metalColorComboBox);
    adjustComboBoxPopupWidth(ui->metalNameComboBox);

    connect(ui->imageLabel1, &ImageClickLabel::rightClicked, this, [=]() {
        QString path = selectAndSaveImage("image1");
        if (!path.isEmpty()) {
            ui->imageLabel1->setPixmap(QPixmap(path).scaled(ui->imageLabel1->size(), Qt::KeepAspectRatio));
            imagePath1 = path;  // store path to save in DB
        }
    });

    connect(ui->imageLabel2, &ImageClickLabel::rightClicked, this, [=]() {
        QString path = selectAndSaveImage("image2");
        if (!path.isEmpty()) {
            ui->imageLabel2->setPixmap(QPixmap(path).scaled(ui->imageLabel2->size(), Qt::KeepAspectRatio));
            imagePath2 = path;  // store path to save in DB
        }
    });



}

OrderMenu::~OrderMenu()
{
    delete ui;
}

void OrderMenu::setSellerInfo(const QString &name, const QString &id)
{
    ui->sellerNameLineEdit->setText(name);
    ui->sellerIdLineEdit->setText(id);
    ui->sellerNameLineEdit->setReadOnly(true);
    ui->sellerIdLineEdit->setReadOnly(true);

    currentSellerId = id;
    currentSellerName = name;
}

int OrderMenu::getNextJobNumber()
{
    QSqlQuery query;
    query.prepare(R"(SELECT jobNo FROM "OrderBook-Detail" WHERE jobNo LIKE 'JOB%' ORDER BY id DESC LIMIT 1)");
    if (query.exec() && query.next()) {
        QString lastJobNo = query.value(0).toString();  // e.g., "JOB00023"
        bool ok;
        int number = lastJobNo.mid(3).toInt(&ok);       // strip "JOB" → 23
        if (ok) return number + 1;
    }
    return 1;  // start from JOB00001
}

int OrderMenu::getNextOrderNumberForSeller(const QString& sellerId)
{
    QSqlQuery query;
    query.prepare(R"(SELECT orderNo FROM "OrderBook-Detail"
                     WHERE orderNo LIKE ? ORDER BY id DESC LIMIT 1)");
    query.addBindValue(sellerId + "%");

    if (query.exec() && query.next()) {
        QString lastOrder = query.value(0).toString(); // e.g., SELL12300007
        QString numericPart = lastOrder.mid(sellerId.length()); // "00007"
        bool ok;
        int number = numericPart.toInt(&ok);
        if (ok) return number + 1;
    }
    return 1;
}



void OrderMenu::insertDummyOrder()
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    // qDebug()<<"we are inside insertion of dummy order";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB:" << db.lastError().text();
    } else {
        qDebug() << "✅ Connected to DB:" << db.databaseName();
    }

    QSqlQuery testQuery;
    testQuery.prepare(R"(
        INSERT INTO "OrderBook-Detail"
        (sellerName, sellerId, partyName, jobNo, orderNo, orderDate, deliveryDate)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");

    testQuery.addBindValue("Test Seller");
    testQuery.addBindValue("SELL123");
    testQuery.addBindValue("TEMP_PARTY");
    testQuery.addBindValue("TEMP_JOB");
    testQuery.addBindValue("TEMP_ORDER");
    testQuery.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
    testQuery.addBindValue(QDate::currentDate().addDays(1).toString("yyyy-MM-dd"));

    if (!testQuery.exec()) {
        QMessageBox::critical(this, "Insert Failed", testQuery.lastError().text());
        qDebug() << "Insert Error:" << testQuery.lastError().text();
        return;
    }

    dummyOrderId = testQuery.lastInsertId().toInt();

    // ✅ Generate new jobNo and orderNo from dummyOrderId
    int jobNum = getNextJobNumber();
    QString finalJobNo = QString("JOB%1").arg(jobNum, 5, 10, QChar('0'));
    int orderNum = getNextOrderNumberForSeller(currentSellerId);
    QString finalOrderNo = QString("%1%2").arg(currentSellerId).arg(orderNum, 5, 10, QChar('0'));

    // ✅ Update the dummy row with real job/order numbers
    QSqlQuery updateQuery;
    updateQuery.prepare(R"(
        UPDATE "OrderBook-Detail"
        SET jobNo = ?, orderNo = ?
        WHERE id = ?
    )");
    updateQuery.addBindValue(finalJobNo);
    updateQuery.addBindValue(finalOrderNo);
    updateQuery.addBindValue(dummyOrderId);
    updateQuery.exec();

    // ✅ Show them in the UI and lock fields
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
        QSqlQuery cleanup;
        cleanup.prepare("DELETE FROM \"OrderBook-Detail\" WHERE id = :id AND isSaved = 0");
        cleanup.bindValue(":id", dummyOrderId);
        cleanup.exec();
    }
    QDialog::closeEvent(event);
}

void OrderMenu::on_savePushButton_clicked()
{

    QString jobNo = ui->jobNoLineEdit->text();
    QString orderNo = ui->orderNoLineEdit->text();

    // Fetch all remaining fields
    QString sellerName = ui->sellerNameLineEdit->text().trimmed();
    QString sellerId = ui->sellerIdLineEdit->text().trimmed();
    QString orderDate = ui->orderDateDateEdit->date().toString("yyyy-MM-dd");
    QString deliveryDate = ui->deliveryDateDateEdit->date().toString("yyyy-MM-dd");

    QString partyName = ui->partyNameLineEdit->text().trimmed();
    QString clientId = ui->clientIdLineEdit->text().trimmed();
    QString agencyId = ui->agencyIdLineEdit->text().trimmed();
    QString shopId = ui->shopIdLineEdit->text().trimmed();
    QString retailleId = ui->reteailleIdLineEdit->text().trimmed();
    QString starId = ui->starIdLineEdit->text().trimmed();
    QString address = ui->addressLineEdit->text();
    QString city = ui->cityLineEdit->text().trimmed();
    QString state = ui->stateLineEdit->text().trimmed();
    QString country = ui->countryLineEdit->text().trimmed();

    QString productName = ui->productNameLineEdit->text().trimmed();
    QString productPis = ui->productPicLineEdit->text();
    QString approxProductWt = ui->approxProductWtLineEdit->text();
    QString metalPrice = ui->metalPriceLineEdit->text();

    QString metalName = ui->metalNameComboBox->currentText();
    QString metalPurity = ui->metalPurityComboBox->currentText();
    QString metalColor = ui->metalColorComboBox->currentText();

    QString sizeNo = ui->sizeNoLineEdit->text();
    QString sizeMM = ui->sizeMMLineEdit->text();
    QString length = ui->lengthLineEdit->text();
    QString width = ui->widthLineEdit->text();
    QString height = ui->heightLineEdit->text();

    QString diaPacific = ui->diaPacificLineEdit->text().trimmed();
    QString diaPurity = ui->diaPurityLineEdit->text().trimmed();
    QString diaColor = ui->diaColorLineEdit->text().trimmed();
    QString diaPrice = ui->diaPriceLineEdit->text();

    QString stPacific = ui->stPacificLineEdit->text().trimmed();
    QString stPurity = ui->stPurityLineEdit->text().trimmed();
    QString stColor = ui->stColorLineEdit->text().trimmed();
    QString stPrice = ui->stPriceLineEdit->text();

    QString designNo1 = ui->designNoLineEdit1->text().trimmed();
    QString designNo2 = ui->designNoLineEdit2->text().trimmed();

    QString image1Path = imagePath1;
    QString image2Path = imagePath2;

    QString metalCertiName = ui->metalCertiNameComboBox->currentText();
    QStringList metalCertList;
    QStandardItemModel* metalModel = qobject_cast<QStandardItemModel*>(ui->metalCertiTypeComboBox->model());
    for (int i = 0; i < metalModel->rowCount(); ++i) {
        QStandardItem* item = metalModel->item(i);
        if (item->checkState() == Qt::Checked) {
            metalCertList << item->text();
        }
    }
    QString metalCertiType = metalCertList.join(", ");

    QString diaCertiName = ui->diaCertiNameComboBox->currentText();
    QStringList diaCertList;
    QStandardItemModel* diaModel = qobject_cast<QStandardItemModel*>(ui->diaCertiTypeComboBox->model());
    for (int i = 0; i < diaModel->rowCount(); ++i) {
        QStandardItem* item = diaModel->item(i);
        if (item->checkState() == Qt::Checked) {
            diaCertList << item->text();
        }
    }
    QString diaCertiType = diaCertList.join(", ");

    QString pesSaki = ui->pesSakiComboBox->currentText();
    QString chainLock = ui->chainLockComboBox->currentText();
    QString polish = ui->polishComboBox->currentText();

    QString settingLabour = ui->settingLabourComboBox->currentText();
    QString metalStemp = ui->metalStempComboBox->currentText();
    QString paymentMethod = ui->payMethodComboBox->currentText();

    QString totalAmount = ui->totalAmountLineEdit->text();
    QString advance = ui->advanceLineEdit->text();
    QString remaining = ui->remainingLineEdit->text();

    QString note = ui->noteTextEdit->toPlainText().trimmed();
    QString extraDetail = ui->extraNoteTextEdit->toPlainText().trimmed();

    QSqlQuery updateQuery;
    updateQuery.prepare(R"(
    UPDATE "OrderBook-Detail" SET
        sellerName = :sellerName,
        sellerId = :sellerId,
        partyName = :partyName,
        jobNo = :jobNo,
        orderNo = :orderNo,
        clientId = :clientId,
        agencyId = :agencyId,
        shopId = :shopId,
        reteailleId = :retailleId,
        starId = :starId,
        address = :address,
        city = :city,
        state = :state,
        country = :country,
        orderDate = :orderDate,
        deliveryDate = :deliveryDate,
        productName = :productName,
        productPis = :productPis,
        approxProductWt = :approxProductWt,
        metalPrice = :metalPrice,
        metalName = :metalName,
        metalPurity = :metalPurity,
        metalColor = :metalColor,
        sizeNo = :sizeNo,
        sizeMM = :sizeMM,
        length = :length,
        width = :width,
        height = :height,
        diaPacific = :diaPacific,
        diaPurity = :diaPurity,
        diaColor = :diaColor,
        diaPrice = :diaPrice,
        stPacific = :stPacific,
        stPurity = :stPurity,
        stColor = :stColor,
        stPrice = :stPrice,
        designNo1 = :designNo1,
        designNo2 = :designNo2,
        image1Path = :image1Path,
        image2Path = :image2Path,
        metalCertiName = :metalCertiName,
        metalCertiType = :metalCertiType,
        diaCertiName = :diaCertiName,
        diaCertiTyoe = :diaCertiType,
        pesSaki = :pesSaki,
        chainLock = :chainLock,
        polish = :polish,
        settingLebour = :settingLabour,
        metalStemp = :metalStemp,
        paymentMethod = :paymentMethod,
        totalAmount = :totalAmount,
        advance = :advance,
        remaining = :remaining,
        note = :note,
        extraDetail = :extraDetail,
        isSaved = 1
    WHERE id = :id
    )");

    updateQuery.bindValue(":sellerName", sellerName);
    updateQuery.bindValue(":sellerId", sellerId);
    updateQuery.bindValue(":partyName", partyName);
    updateQuery.bindValue(":jobNo", jobNo);
    updateQuery.bindValue(":orderNo", orderNo);
    updateQuery.bindValue(":clientId", clientId);
    updateQuery.bindValue(":agencyId", agencyId);
    updateQuery.bindValue(":shopId", shopId);
    updateQuery.bindValue(":retailleId", retailleId);
    updateQuery.bindValue(":starId", starId);
    updateQuery.bindValue(":address", address);
    updateQuery.bindValue(":city", city);
    updateQuery.bindValue(":state", state);
    updateQuery.bindValue(":country", country);
    updateQuery.bindValue(":orderDate", orderDate);
    updateQuery.bindValue(":deliveryDate", deliveryDate);
    updateQuery.bindValue(":productName", productName);
    updateQuery.bindValue(":productPis", productPis);
    updateQuery.bindValue(":approxProductWt", approxProductWt);
    updateQuery.bindValue(":metalPrice", metalPrice);
    updateQuery.bindValue(":metalName", metalName);
    updateQuery.bindValue(":metalPurity", metalPurity);
    updateQuery.bindValue(":metalColor", metalColor);
    updateQuery.bindValue(":sizeNo", sizeNo);
    updateQuery.bindValue(":sizeMM", sizeMM);
    updateQuery.bindValue(":length", length);
    updateQuery.bindValue(":width", width);
    updateQuery.bindValue(":height", height);
    updateQuery.bindValue(":diaPacific", diaPacific);
    updateQuery.bindValue(":diaPurity", diaPurity);
    updateQuery.bindValue(":diaColor", diaColor);
    updateQuery.bindValue(":diaPrice", diaPrice);
    updateQuery.bindValue(":stPacific", stPacific);
    updateQuery.bindValue(":stPurity", stPurity);
    updateQuery.bindValue(":stColor", stColor);
    updateQuery.bindValue(":stPrice", stPrice);
    updateQuery.bindValue(":designNo1", designNo1);
    updateQuery.bindValue(":designNo2", designNo2);
    updateQuery.bindValue(":image1Path", image1Path);
    updateQuery.bindValue(":image2Path", image2Path);
    updateQuery.bindValue(":metalCertiName", metalCertiName);
    updateQuery.bindValue(":metalCertiType", metalCertiType);
    updateQuery.bindValue(":diaCertiName", diaCertiName);
    updateQuery.bindValue(":diaCertiType", diaCertiType);
    updateQuery.bindValue(":pesSaki", pesSaki);
    updateQuery.bindValue(":chainLock", chainLock);
    updateQuery.bindValue(":polish", polish);
    updateQuery.bindValue(":settingLabour", settingLabour);
    updateQuery.bindValue(":metalStemp", metalStemp);
    updateQuery.bindValue(":paymentMethod", paymentMethod);
    updateQuery.bindValue(":totalAmount", totalAmount);
    updateQuery.bindValue(":advance", advance);
    updateQuery.bindValue(":remaining", remaining);
    updateQuery.bindValue(":note", note);
    updateQuery.bindValue(":extraDetail", extraDetail);
    updateQuery.bindValue(":id", dummyOrderId);  // 👈 Make sure dummyOrderId is correct


    if (!updateQuery.exec()) {
        QMessageBox::critical(this, "Update Failed", updateQuery.lastError().text());
    } else {
        QMessageBox::information(this, "Success", "Order saved successfully.");
        isSaved = true;  // ✅ mark saved
    }
}
