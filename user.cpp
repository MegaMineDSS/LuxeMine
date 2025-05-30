#include "user.h"
#include "ui_user.h"
#include "DatabaseUtils.h"
#include "BackupUtils.h"
#include "PdfUtils.h"
#include "Utils.h"
#include "cartitemwidget.h"
#include <QMessageBox>
#include <QPixmap>
#include <QTimer>
#include <QRegularExpression>
#include <QDir>
#include <QFileDialog>

User::User(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::User)
    , dbManager()
    , currentImageIndex(0)
    , diamondTable(nullptr)
    , stoneTable(nullptr)
    , cartItemsContainer(nullptr)
    , currentUserId("")
{
    ui->setupUi(this);
    setupUi();
    loadData();
    setupMobileComboBox();
}

User::~User()
{
    delete diamondTable;
    delete stoneTable;
    delete ui;
}

void User::setupUi()
{
    setWindowSize(this);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowTitle("User");
    setWindowIcon(QIcon(":/icon/homeIcon.png"));

    connect(ui->goldCombobox, &QComboBox::currentTextChanged, this, &User::updateGoldWeight);
    connect(ui->cartMainpage, &QPushButton::clicked, this, &User::on_cartMainpage_clicked);
    connect(ui->backMainpage, &QPushButton::clicked, this, &User::on_backMainpage_clicked);

    QDir::setCurrent(QCoreApplication::applicationDirPath());

    diamondTable = new QTableWidget(this);
    diamondTable->setWindowFlags(Qt::ToolTip);
    diamondTable->setColumnCount(3);
    diamondTable->setHorizontalHeaderLabels({"Type", "SizeMM", "Quantity"});
    diamondTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diamondTable->hide();

    stoneTable = new QTableWidget(this);
    stoneTable->setWindowFlags(Qt::ToolTip);
    stoneTable->setColumnCount(3);
    stoneTable->setHorizontalHeaderLabels({"Type", "SizeMM", "Quantity"});
    stoneTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stoneTable->hide();

    ui->diamond_detail->installEventFilter(this);
    ui->stone_detail->installEventFilter(this);

    cartItemsContainer = ui->scrollArea->widget();
    if (!cartItemsContainer) {
        cartItemsContainer = new QWidget;
        ui->scrollArea->setWidget(cartItemsContainer);
    }
    cartItemsContainer->setLayout(new QVBoxLayout);
    ui->scrollArea->setWidgetResizable(true);

    ui->goldFinaldetail->setText("Total Gold Weight: 0.00g");
}

void User::setupMobileComboBox() {
    // List of country codes
    QMap<QString, QString> countryCodes;
    countryCodes["+1"] = "United States";
    countryCodes["+44"] = "United Kingdom";
    countryCodes["+91"] = "India";
    countryCodes["+81"] = "Japan";
    countryCodes["+86"] = "China";
    countryCodes["+33"] = "France";
    countryCodes["+49"] = "Germany";
    // Add more country codes as needed

    ui->mobileall->clear();
    ui->mobileall->addItems(countryCodes.keys());
    ui->mobileall->setCurrentIndex(2); // Default to first country code
}

void User::loadData()
{
    imageRecords = dbManager.getAllItems();
    currentImageIndex = 0;

    if (!imageRecords.isEmpty()) {
        loadImage(currentImageIndex);
    } else {
        QMessageBox::warning(this, "No Data", "No image data found in the database.");
    }
}

void User::loadImage(int index)
{
    if (index < 0 || index >= imageRecords.size()) return;

    currentImageIndex = index;
    const ImageRecord &record = imageRecords[index];

    ui->companyName->setText(record.companyName);
    ui->designNo->setText("<u>Design No.</u><br> " + record.designNo);
    ui->label_slneNo->setText("Slne No.\nSLNE" + QString::number(record.imageId));

    if (QFile::exists(record.imagePath)) {
        QPixmap pixmap(record.imagePath);
        QTimer::singleShot(100, this, [=]() {
            ui->image_viewUser->setPixmap(pixmap.scaled(ui->image_viewUser->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        });
    } else {
        QMessageBox::warning(this, "Image Error", "Image not found: " + record.imagePath);
    }

    goldData = DatabaseUtils::parseGoldJson(record.goldJson);
    ui->goldCombobox->clear();
    for (auto it = goldData.constBegin(); it != goldData.constEnd(); ++it) {
        ui->goldCombobox->addItem(it.key());
    }

    updateGoldWeight();
    displayDiamondDetails();
    displayStoneDetails();
    ui->itemSelectionSpinBox->setValue(0);
}

void User::on_nextImage_clicked() {
    if (currentImageIndex < imageRecords.size() - 1) {
        loadImage(++currentImageIndex);
    }
}

void User::on_previousImage_clicked() {
    if (currentImageIndex > 0) {
        loadImage(--currentImageIndex);
    }
}

void User::updateGoldWeight() {
    QString selectedKts = ui->goldCombobox->currentText();
    if (goldData.contains(selectedKts)) {
        double value = goldData[selectedKts].toDouble();
        ui->goldWeight->setText("Gold Weight: \n\t" +  QString::number(value, 'f', 3));
    } else {
        ui->goldWeight->setText("Gold Weight: N/A");
    }
}

void User::on_pushButton_clicked()
{
    if (saveOrLoadUser()) {
        ui->stackedWidget->setCurrentIndex(1);
        loadData();
    }
}

bool User::saveOrLoadUser()
{
    QString userId = ui->userid->text().trimmed();
    QString mobilePrefix = ui->mobileall->currentText().trimmed();
    QString mobileNo = ui->mobileNouser->text().trimmed();
    QString name = ui->nameuser->text().trimmed();
    QString companyName = ui->companyNameuser->text().trimmed();
    QString gstNo = ui->gstnouser->text().trimmed();
    QString emailId = ui->mailiduser->text().trimmed();
    QString address = ui->addressuser->toPlainText().trimmed();

    // Check if userId is provided
    if (!userId.isEmpty()) {
        if (DatabaseUtils::userExists(userId)) {
            currentUserId = userId;
            loadUserCart(userId);
            return true;
        } else {
            QMessageBox::warning(this, "User Not Found", "The provided User ID does not exist.");
            return false;
        }
    }

    // Validate mobile number and name for new user
    if (mobileNo.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Mobile number and name are required.");
        return false;
    }

    QRegularExpression mobileRegex(R"(\d{7,15})");
    if (!mobileRegex.match(mobileNo).hasMatch()) {
        QMessageBox::warning(this, "Input Error", "Mobile number must be 7-15 digits.");
        return false;
    }

    // Check company name for new user
    if (companyName.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Company name is required for new user.");
        return false;
    }

    // Validate email if provided
    if (!emailId.isEmpty()) {
        QRegularExpression emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        if (!emailRegex.match(emailId).hasMatch()) {
            QMessageBox::warning(this, "Input Error", "Invalid email format.");
            return false;
        }
    }

    // Generate userId from mobile prefix and number
    userId = mobilePrefix + mobileNo;

    // Check if user exists with mobile number and name
    if (DatabaseUtils::userExistsByMobileAndName(userId, name)) {
        currentUserId = userId;
        loadUserCart(userId);
        return true;
    }

    // Insert new user
    if (DatabaseUtils::insertUser(userId, companyName, userId, gstNo, name, emailId, address)) {
        QMessageBox::information(this, "Success", QString("User %1 created successfully.").arg(userId));
        currentUserId = userId;
        loadUserCart(userId);
        return true;
    } else {
        QMessageBox::critical(this, "Database Error", "Failed to create new user.");
        return false;
    }
}

void User::on_cartMainpage_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    updateCartDisplay();
}

void User::on_backMainpage_clicked() {
    ui->stackedWidget->setCurrentIndex(1);
    loadData();
}

bool User::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->diamond_detail) {
        if (event->type() == QEvent::Enter && !currentDiamondJson.isEmpty()) {
            QJsonArray array = DatabaseUtils::parseJsonArray(currentDiamondJson);
            diamondTable->setRowCount(array.size());

            for (int i = 0; i < array.size(); ++i) {
                QJsonObject obj = array[i].toObject();
                diamondTable->setItem(i, 0, new QTableWidgetItem(obj["type"].toString().trimmed()));
                diamondTable->setItem(i, 1, new QTableWidgetItem(obj["sizeMM"].toString().trimmed()));
                diamondTable->setItem(i, 2, new QTableWidgetItem(obj["quantity"].toString()));
            }

            diamondTable->resizeColumnsToContents();
            diamondTable->resizeRowsToContents();
            diamondTable->move(ui->diamond_detail->mapToGlobal(QPoint(0, ui->diamond_detail->height())));
            diamondTable->show();
        } else if (event->type() == QEvent::Leave) {
            diamondTable->hide();
        }
    } else if (obj == ui->stone_detail) {
        if (event->type() == QEvent::Enter && !currentStoneJson.isEmpty()) {
            QJsonArray array = DatabaseUtils::parseJsonArray(currentStoneJson);
            stoneTable->setRowCount(array.size());

            for (int i = 0; i < array.size(); ++i) {
                QJsonObject obj = array[i].toObject();
                stoneTable->setItem(i, 0, new QTableWidgetItem(obj["type"].toString().trimmed()));
                stoneTable->setItem(i, 1, new QTableWidgetItem(obj["sizeMM"].toString().trimmed()));
                stoneTable->setItem(i, 2, new QTableWidgetItem(obj["quantity"].toString()));
            }

            stoneTable->resizeColumnsToContents();
            stoneTable->resizeRowsToContents();
            stoneTable->move(ui->stone_detail->mapToGlobal(QPoint(0, ui->stone_detail->height())));
            stoneTable->show();
        } else if (event->type() == QEvent::Leave) {
            stoneTable->hide();
        }
    }

    return QDialog::eventFilter(obj, event);
}

void User::displayDiamondDetails()
{
    if (currentImageIndex < 0 || currentImageIndex >= imageRecords.size()) {
        ui->diamond_detail->setText("Diamond Wt.\nNo image selected");
        return;
    }

    int imageId = imageRecords[currentImageIndex].imageId;
    auto result = DatabaseUtils::fetchDiamondDetails(imageId);
    currentDiamondJson = result.first;

    if (result.second.isEmpty()) {
        ui->diamond_detail->setText("Diamond Wt.\nNo diamond data");
        return;
    }

    ui->diamond_detail->setText("Diamond Wt.\n" + result.second.trimmed());
}

void User::displayStoneDetails()
{
    if (currentImageIndex < 0 || currentImageIndex >= imageRecords.size()) {
        ui->stone_detail->setText("Stone Wt.\nNo image selected");
        return;
    }

    int imageId = imageRecords[currentImageIndex].imageId;
    auto result = DatabaseUtils::fetchStoneDetails(imageId);
    currentStoneJson = result.first;

    if (result.second.isEmpty()) {
        ui->stone_detail->setText("Stone Wt.\nNo stone data");
        return;
    }

    ui->stone_detail->setText("Stone Wt.\n" + result.second.trimmed());
}

void User::on_selectButton_clicked()
{
    if (currentImageIndex < 0 || currentImageIndex >= imageRecords.size()) {
        QMessageBox::warning(this, "Selection Error", "No image selected.");
        return;
    }

    int imageId = imageRecords[currentImageIndex].imageId;
    QString goldType = ui->goldCombobox->currentText();
    int itemCount = ui->itemSelectionSpinBox->value();

    if (itemCount <= 0) {
        QMessageBox::warning(this, "Selection Error", "Please select at least one item.");
        return;
    }

    // Create new selection
    SelectionData selection;
    selection.imageId = imageId;
    selection.goldType = goldType;
    selection.itemCount = itemCount;
    selection.diamondJson = DatabaseUtils::fetchJsonData(imageId, "diamond");
    selection.stoneJson = DatabaseUtils::fetchJsonData(imageId, "stone");
    selection.pdf_path = QString("/path/to/pdf_%1.pdf").arg(imageId); // Replace with actual PDF path logic

    // Debug selection data
    qDebug() << "Selection: imageId =" << imageId
             << ", goldType =" << goldType
             << ", itemCount =" << itemCount
             << ", diamondJson =" << selection.diamondJson
             << ", stoneJson =" << selection.stoneJson
             << ", pdf_path =" << selection.pdf_path;

    // Remove existing selection with same imageId and goldType
    for (int i = selections.size() - 1; i >= 0; --i) {
        if (selections[i].imageId == imageId && selections[i].goldType == goldType) {
            selections.removeAt(i);
        }
    }

    // Add new selection
    selections.append(selection);

    // Save to database if user is logged in
    if (!currentUserId.isEmpty()) {
        saveCartToDatabase();
    }

    updateCartDisplay();
}

void User::updateCartDisplay()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(cartItemsContainer->layout());
    if (!layout) return;

    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) widget->deleteLater();
        delete item;
    }

    for (const SelectionData &selection : selections) {
        QPixmap image = DatabaseUtils::fetchImagePixmap(selection.imageId);
        CartItemWidget *itemWidget = new CartItemWidget(selection.imageId, selection.goldType, selection.itemCount, image, cartItemsContainer);
        layout->addWidget(itemWidget);

        connect(itemWidget, &CartItemWidget::quantityChanged, this, &User::on_cartItemQuantityChanged);
        connect(itemWidget, &CartItemWidget::removeRequested, this, &User::on_cartItemRemoveRequested);
    }

    layout->addStretch();
    updateGoldSummary();
    updateDiamondSummary();
    updateStoneSummary();
}

void User::updateGoldSummary()
{
    double totalGoldWeight = DatabaseUtils::calculateTotalGoldWeight(selections);
    ui->goldFinaldetail->setText(QString("Total Gold Weight: %1g").arg(totalGoldWeight, 0, 'f', 3));
}

void User::updateDiamondSummary()
{
    DatabaseUtils::updateSummaryTable(ui->diamondFinaldetail, selections, "diamond");
}

void User::updateStoneSummary()
{
    DatabaseUtils::updateSummaryTable(ui->stoneFinaldetail, selections, "stone");
}

void User::on_cartItemQuantityChanged(int imageId, const QString &goldType, int newQuantity)
{
    for (SelectionData &selection : selections) {
        if (selection.imageId == imageId && selection.goldType == goldType) {
            selection.itemCount = newQuantity;
            break;
        }
    }

    saveCartToDatabase();
    updateCartDisplay();
}

void User::on_cartItemRemoveRequested(int imageId, const QString &goldType)
{
    for (int i = selections.size() - 1; i >= 0; --i) {
        if (selections[i].imageId == imageId && selections[i].goldType == goldType) {
            selections.remove(i);
            break;
        }
    }

    saveCartToDatabase();
    updateCartDisplay();
}

void User::on_makePdfButton_clicked()
{
    if (currentUserId.isEmpty()) {
        QMessageBox::warning(this, "Error", "No user logged in. Please log in first.");
        return;
    }

    if (selections.isEmpty()) {
        QMessageBox::warning(this, "Error", "No items selected to generate PDF.");
        return;
    }

    QDir pdfDir(QCoreApplication::applicationDirPath());
    if (!pdfDir.exists("pdfs")) {
        pdfDir.mkdir("pdfs");
    }
    QString sanitizedUserId = currentUserId;
    sanitizedUserId.remove('+');
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss");
    QString pdfFileName = QString("%1_%2.pdf").arg(currentTime, sanitizedUserId);
    QString pdfPath = pdfDir.filePath(QString("pdfs/%1").arg(pdfFileName));

    qDebug() << "Generating PDF at path:" << pdfPath << "for userId:" << currentUserId;

    if (PdfUtils::generateCartPdf(pdfPath, currentUserId, ui->nameuser->text().trimmed(),
                                  ui->companyNameuser->text().trimmed(),
                                  ui->mobileall->currentText() + ui->mobileNouser->text().trimmed(),
                                  selections, cartItemsContainer, ui->diamondFinaldetail,
                                  ui->stoneFinaldetail, ui->goldFinaldetail->text())) {
        QMessageBox::information(this, "Success", "PDF generated successfully at: " + pdfPath);

        // Update pdf_path in selections with the new PDF path
        for (SelectionData &selection : selections) {

            selection.pdf_path = pdfPath;
        }

        if (!DatabaseUtils::saveUserCart(currentUserId, selections)) {
            QMessageBox::critical(this, "Database Error", "Failed to save PDF path to cart. Check console for details.");
        } else {
            qDebug() << "PDF path" << pdfPath << "saved successfully for user:" << currentUserId;
        }
    } else {
        QMessageBox::critical(this, "Error", "Failed to generate PDF at: " + pdfPath);
    }
}

void User::on_rbackup_clicked()
{
    QString zipPath = QFileDialog::getOpenFileName(this, "Import Catalog Backup", "", "ZIP Files (*.zip)");
    if (zipPath.isEmpty()) return;

    if (BackupUtils::importAdminBackup(zipPath)) {
        QMessageBox::information(this, "Success", "Catalog and images imported successfully!");
        loadData();
    } else {
        QMessageBox::critical(this, "Error", "Failed to import catalog backup.");
    }
}

void User::on_cbackup_clicked()
{
    if (currentUserId.isEmpty()) {
        QMessageBox::warning(this, "Error", "No user logged in.");
        return;
    }

    // Update the filter to expect a .zip file
    QString filePath = QFileDialog::getSaveFileName(this, "Export Cart Backup", "", "Zip Archive (*.zip)");
    if (filePath.isEmpty()) return;

    // Call exportUserBackup with the selected file path
    if (BackupUtils::exportUserBackup(filePath, currentUserId)) {
        // Adjust the success message to reflect the actual file path (in case .zip was appended)
        QString actualFilePath = filePath;
        if (!actualFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
            actualFilePath += ".zip";
        }
        QMessageBox::information(this, "Success", "Cart backup exported to: " + actualFilePath);
    } else {
        QMessageBox::critical(this, "Error", "Failed to create cart backup.");
    }
}

void User::loadUserCart(const QString &userId)
{
    selections = DatabaseUtils::loadUserCart(userId);
    updateCartDisplay();
}

void User::saveCartToDatabase()
{
    if (currentUserId.isEmpty()) {
        QMessageBox::warning(this, "Cart Error", "No user logged in. Cannot save cart.");
        return;
    }

    if (selections.isEmpty()) {
        qDebug() << "No selections to save for user:" << currentUserId;
        return;
    }

    if (!DatabaseUtils::saveUserCart(currentUserId, selections)) {
        QMessageBox::critical(this, "Database Error", "Failed to save cart data. Check console for details.");
    } else {
        qDebug() << "Cart saved successfully for user:" << currentUserId;
    }
}
