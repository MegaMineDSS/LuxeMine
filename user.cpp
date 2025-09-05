#include "user.h"
#include "ui_user.h"

#include <QMessageBox>
#include <QPixmap>
#include <QTimer>
#include <QRegularExpression>
#include <QDir>
#include <QCompleter>

#include "databaseutils.h"
#include "PdfUtils.h"
#include "utils.h"
#include "cartitemwidget.h"



void User::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    QPixmap bg(":/Backgrounds/Client-details.png");
    if (!bg.isNull()) {
        bg = bg.scaled(ui->page->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPalette pal;
        pal.setBrush(ui->page->backgroundRole(), bg);
        ui->page->setAutoFillBackground(true);
        ui->page->setPalette(pal);

        ui->page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

}

User::User(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::User)
    //, dbManager()
    , currentImageIndex(0)
    , diamondTable(nullptr)
    , stoneTable(nullptr)
    , cartItemsContainer(nullptr)
    , currentUserId("")
{
    ui->setupUi(this);
    setupUi();

    // For backgrund recaclulation so that resizeEvent function's effect can be seen otherwise you have to resize window to see the background effect performed in resiveEvent function.
    QTimer::singleShot(0, this, [this]() {
        resizeEvent(nullptr);  // force one background recalculation
    });


    // loadData();
    setupMobileComboBox();
}

User::~User()
{
    // delete diamondTable;
    // delete stoneTable;
    delete ui;
}


void User::setupUi()
{
    setWindowSize(this);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowTitle("User");
    setWindowIcon(QIcon(":/icon/homeIcon.png"));
    connect(ui->cartMainpage, &QPushButton::clicked, this, &User::on_cartMainpage_clicked);
    connect(ui->backMainpage, &QPushButton::clicked, this, &User::on_backMainpage_clicked);

    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // diamondTable
    diamondTable = new QTableWidget(this);
    diamondTable->setWindowFlags(Qt::ToolTip);
    diamondTable->setColumnCount(3);
    diamondTable->setHorizontalHeaderLabels({"Type", "SizeMM", "Quantity"});
    diamondTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diamondTable->hide();

    // stoneTable
    stoneTable = new QTableWidget(this);
    stoneTable->setWindowFlags(Qt::ToolTip);
    stoneTable->setColumnCount(3);
    stoneTable->setHorizontalHeaderLabels({"Type", "SizeMM", "Quantity"});
    stoneTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    stoneTable->hide();

    ui->diamond_detail->installEventFilter(this);
    ui->stone_detail->installEventFilter(this);

    // cartItemsContainer
    cartItemsContainer = ui->scrollArea->widget();
    if (!cartItemsContainer) {
        cartItemsContainer = new QWidget(this);   // ✅ give it a parent
        ui->scrollArea->setWidget(cartItemsContainer);
    }
    cartItemsContainer->setLayout(new QVBoxLayout(cartItemsContainer));  // ✅ parent layout properly
    ui->scrollArea->setWidgetResizable(true);

    ui->goldFinaldetail->setText("Total Gold Weight: 0.00g");

    loadData();

}

void User::setupMobileComboBox() {
    QFile file(":/json_files/country_codes.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open country_codes.json";
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonObject countryCodes = doc.object();
    ui->mobileall->clear();

    for (auto it = countryCodes.begin(); it != countryCodes.end(); ++it) {
        const QString code = it.key();
        const QString country = it.value().toString();
        const QString display = code + " (" + country + ")";
        ui->mobileall->addItem(display, code);
        ui->mobileall->setItemData(ui->mobileall->count() - 1, country, Qt::UserRole + 1);
    }

    ui->mobileall->setEditable(true);
    ui->mobileall->setInsertPolicy(QComboBox::NoInsert);
    ui->mobileall->lineEdit()->setPlaceholderText("Search by code or country…");
    ui->mobileall->lineEdit()->setClearButtonEnabled(true);

    // 🔑 Prevent completer leak
    if (ui->mobileall->completer())
        ui->mobileall->completer()->deleteLater();

    QCompleter *completer = new QCompleter(ui->mobileall->model(), ui->mobileall);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    ui->mobileall->setCompleter(completer);

    connect(completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &User::selectMobileCodeFromText);
    connect(ui->mobileall->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
        selectMobileCodeFromText(ui->mobileall->lineEdit()->text());
    });

    int idx = ui->mobileall->findData("+91");
    if (idx != -1) {
        ui->mobileall->setCurrentIndex(idx);
        ui->mobileall->setEditText(ui->mobileall->itemText(idx));
    }
}

void User::selectMobileCodeFromText(const QString &text)
{
    QString t = text.trimmed();

    // Try matching by code first (accept "+91" or "91")
    QString codeCandidate = t;
    codeCandidate.remove(' ');
    if (!codeCandidate.isEmpty() && (codeCandidate[0].isDigit() || codeCandidate.startsWith('+'))) {
        if (!codeCandidate.startsWith('+'))
            codeCandidate.prepend('+');
        int idx = ui->mobileall->findData(codeCandidate);
        if (idx != -1) {
            ui->mobileall->setCurrentIndex(idx);
            ui->mobileall->setEditText(ui->mobileall->itemText(idx));
            return;
        }
    }

    // Try exact country match via stored role
    const int count = ui->mobileall->count();
    for (int i = 0; i < count; ++i) {
        const QString country = ui->mobileall->itemData(i, Qt::UserRole + 1).toString();
        if (country.compare(t, Qt::CaseInsensitive) == 0) {
            ui->mobileall->setCurrentIndex(i);
            ui->mobileall->setEditText(ui->mobileall->itemText(i));
            return;
        }
    }

    // Fallback: substring match against display text "+code (Country)"
    for (int i = 0; i < count; ++i) {
        const QString display = ui->mobileall->itemText(i);
        if (display.contains(t, Qt::CaseInsensitive)) {
            ui->mobileall->setCurrentIndex(i);
            ui->mobileall->setEditText(display);
            return;
        }
    }
}

void User::loadData()
{
    imageRecords = DatabaseUtils::getAllItems();
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

        // ✅ capture a pre-scaled copy so we don't use a dangling local
        QPixmap scaled = pixmap.scaled(ui->image_viewUser->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);

        QTimer::singleShot(100, this, [this, scaled]() {
            ui->image_viewUser->setPixmap(scaled);
        });
    } else {
        QMessageBox::warning(this, "Image Error", "Image not found: " + record.imagePath);
    }



    updateGoldWeight(record.goldJson);
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

void User::updateGoldWeight(const QString &goldJson) {
    goldData = DatabaseUtils::parseGoldJson(goldJson);

    QMenu *filterMenu = new QMenu(this);
    QMenu *goldMenu = filterMenu->addMenu("Gold");

    bool firstSet = false;

    for (auto it = goldData.constBegin(); it != goldData.constEnd(); ++it) {
        QString karat = it.key();
        double weight = it.value().toDouble();

        QAction *action = goldMenu->addAction(karat);
        connect(action, &QAction::triggered, this, [this, karat, weight]() {
            currentGoldSelection = karat;
            ui->goldWeight->setText(
                QString("Gold Weight: \n\t%1").arg(QString::number(weight, 'f', 3))
                );
        });

        // ✅ Set first value by default (so when image changes, something is shown immediately)
        if (!firstSet) {
            currentGoldSelection = karat;
            ui->goldWeight->setText(
                QString("Gold Weight: \n\t%1").arg(QString::number(weight, 'f', 3))
                );
            firstSet = true;
        }
    }

    // Attach menu to button
    ui->user_filter_btn->setMenu(filterMenu);
}

void User::on_pushButton_clicked()
{
    if (saveOrLoadUser()) {
        ui->stackedWidget->setCurrentIndex(2);
        loadData();
    }
}

bool User::saveOrLoadUser()
{
    QString userId = ui->userid->text().trimmed();
    QString mobilePrefix = ui->mobileall->currentData().toString().trimmed();  // ✅ FIX: fetch actual code, not display text
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
    ui->stackedWidget->setCurrentIndex(3);
    updateCartDisplay();
}

void User::on_backMainpage_clicked() {
    ui->stackedWidget->setCurrentIndex(2);
    loadData();
}

bool User::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->diamond_detail) {
        if (event->type() == QEvent::Enter && !currentDiamondJson.isEmpty()) {
            QJsonArray array = DatabaseUtils::parseJsonArray(currentDiamondJson);

            diamondTable->clearContents();                  // clear old items
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

            stoneTable->clearContents();                   // ✅ clear old items
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
    // qDebug()<<currentDiamondJson;
    // qDebug()<<result.second.trimmed();
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
    QString goldType = currentGoldSelection;
    int itemCount = ui->itemSelectionSpinBox->value();

    if (itemCount <= 0) {
        QMessageBox::warning(this, "Selection Error", "Please select at least one item.");
        return;
    }

    SelectionData selection;
    selection.imageId = imageId;
    selection.goldType = goldType;
    selection.itemCount = itemCount;
    selection.diamondJson = DatabaseUtils::fetchJsonData(imageId, "diamond");
    selection.stoneJson   = DatabaseUtils::fetchJsonData(imageId, "stone");

    // ✅ Generate real PDF path
    // QString pdfDir = QDir(QCoreApplication::applicationDirPath()).filePath("pdfs");
    // QDir().mkpath(pdfDir);
    // selection.pdf_path = QDir(pdfDir).filePath(QString("pdf_%1.pdf").arg(imageId));
    selection.pdf_path = "";

    // Remove duplicate entry
    for (int i = selections.size() - 1; i >= 0; --i) {
        if (selections[i].imageId == imageId && selections[i].goldType == goldType) {
            selections.removeAt(i);
        }
    }

    selections.append(selection);

    if (!currentUserId.isEmpty()) {
        saveCartToDatabase();
    }

    updateCartDisplay();
}

void User::updateCartDisplay()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(cartItemsContainer->layout());
    if (!layout) return;

    // Clear existing widgets
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr);   // ✅ detach from layout immediately
            widget->deleteLater();        // ✅ schedule deletion
        }
        delete item;
    }

    // Add cart items
    for (const SelectionData &selection : selections) {
        QPixmap image = DatabaseUtils::fetchImagePixmap(selection.imageId);
        auto *itemWidget = new CartItemWidget(
            selection.imageId,
            selection.goldType,
            selection.itemCount,
            image,
            cartItemsContainer
            );
        layout->addWidget(itemWidget);

        connect(itemWidget, &CartItemWidget::quantityChanged,
                this, &User::on_cartItemQuantityChanged);
        connect(itemWidget, &CartItemWidget::removeRequested,
                this, &User::on_cartItemRemoveRequested);
    }

    layout->addStretch();

    // Update summaries
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

    // Persist + refresh summaries only (no UI rebuild)
    saveCartToDatabase();
    updateGoldSummary();
    updateDiamondSummary();
    updateStoneSummary();
}

void User::on_cartItemRemoveRequested(int imageId, const QString &goldType)
{
    // 🔹 Remove from selections
    for (int i = selections.size() - 1; i >= 0; --i) {
        if (selections[i].imageId == imageId && selections[i].goldType == goldType) {
            selections.removeAt(i);
            break;
        }
    }

    // 🔹 Remove widget directly
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(cartItemsContainer->layout());
    if (layout) {
        for (int i = 0; i < layout->count(); ++i) {
            QWidget *widget = layout->itemAt(i)->widget();
            if (CartItemWidget *itemWidget = qobject_cast<CartItemWidget*>(widget)) {
                if (itemWidget->getImageId() == imageId &&
                    itemWidget->getGoldType() == goldType) {
                    layout->removeWidget(itemWidget);
                    itemWidget->deleteLater(); // safe cleanup
                    break;
                }
            }
        }
    }

    // 🔹 Save updated cart
    saveCartToDatabase();

    // 🔹 Just update summaries, not whole UI
    updateGoldSummary();
    updateDiamondSummary();
    updateStoneSummary();
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

    if (!DatabaseUtils::saveUserCart(currentUserId, selections)) {
        QMessageBox::critical(this, "Database Error", "Failed to save cart data. Check console for details.");
    } else {
        qDebug() << "Cart saved successfully for user:" << currentUserId;
    }
}




void User::on_make_pdf_Mainpage_clicked()
{

}


void User::on_user_register_redirect_button_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    resizeEvent(nullptr); // Force background recalculation so that background image properly set
}

