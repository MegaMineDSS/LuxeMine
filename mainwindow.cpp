#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QRandomGenerator>
#include <QScopedPointer>
#include <QMessageBox>
#include <QMdiSubWindow>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , newAdmin(nullptr)
    , newUser(nullptr)
    , newAddCatalog(nullptr)
    , newOrderMenu(nullptr)
    , loginWindow(nullptr)
    , newOrderList(nullptr)
{
    ui->setupUi(this);



    window()->setWindowTitle("Home");
    window()->setWindowIcon(QIcon(":/icon/homeIcon.png"));

    ui->backgroundLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->backgroundLabel->setScaledContents(true);  // Important: Let QLabel handle scaling
    ui->backgroundLabel->setMinimumSize(200, 350);


    setRandomBackground();  // Call after setup

    // Example connections for Admin menu
    connect(ui->actionShow_Image, &QAction::triggered, this, [this]() {
        openAdminPage(0); // index for show_images
    });

    connect(ui->actionUpdate_Price, &QAction::triggered, this, [this]() {
        openAdminPage(1);
    });

    connect(ui->actionAdd_Diamond, &QAction::triggered, this, [this]() {
        openAdminPage(2);
    });

    connect(ui->actionShow_Users, &QAction::triggered, this, [this]() {
        openAdminPage(3);
    });

    connect(ui->actionJewellry_Menu, &QAction::triggered, this, [this]() {
        openAdminPage(4);
    });

    connect(ui->actionOrder_Book_Users, &QAction::triggered, this, [this]() {
        openAdminPage(5);
    });

    connect(ui->actionOrder_List, &QAction::triggered, this, [this]() {
        openAdminPage(6);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if(newAdmin && !newAdmin->isHidden()){
        newAdmin->raise();
        newAdmin->activateWindow();
    }
    else{
        newAdmin = new Admin();
        newAdmin->setAttribute(Qt::WA_DeleteOnClose);
        connect(newAdmin, &QObject::destroyed, this, [this]() { newAdmin = nullptr; });
        newAdmin->show();
    }
}

void MainWindow::openAdminPage(int pageIndex)
{
    if (!newAdmin || newAdmin->isHidden()) {
        newAdmin = new Admin();
        newAdmin->setAttribute(Qt::WA_DeleteOnClose);
        connect(newAdmin, &QObject::destroyed, this, [this]() { newAdmin = nullptr; });
        newAdmin->show();
    }

    newAdmin->raise();
    newAdmin->activateWindow();

    // directly set page
    // newAdmin->setPage(pageIndex);
    newAdmin->setRequestedPage(pageIndex);
}

void MainWindow::on_pushButton_2_clicked()
{
    if (newUser && !newUser->isHidden()) {
        newUser->raise();
        newUser->activateWindow();
    } else {
        newUser = new User();
        newUser->setAttribute(Qt::WA_DeleteOnClose);  // auto-delete when closed

        // Reset pointer when the window is destroyed
        connect(newUser, &QObject::destroyed, this, [this]() { newUser = nullptr; });

        newUser->show();
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if (newAddCatalog && !newAddCatalog->isHidden()) {
        newAddCatalog->raise();
        newAddCatalog->activateWindow();
    } else {
        newAddCatalog = new AddCatalog();   // give MainWindow as parent
        newAddCatalog->setAttribute(Qt::WA_DeleteOnClose);  // auto-delete when closed

        // Reset pointer when destroyed
        connect(newAddCatalog, &QObject::destroyed, this, [this]() {
            newAddCatalog = nullptr;
        });

        newAddCatalog->show();
    }
}

void MainWindow::on_orderBookButton_clicked()
{
    // Stack-based dialog, safe because exec() blocks until closed
    LoginWindow loginWindow(this);

    connect(&loginWindow, &LoginWindow::loginAccepted, this,
            [this, &loginWindow](const QString &action)
            {
                QString role = loginWindow.getRole().toLower();
                QString userName = loginWindow.getUserName();
                QString userId = loginWindow.getUserId();

                if (action == "orderMenu" && role == "seller") {
                    QString partyId = loginWindow.getPartyId();
                    QString partyName = loginWindow.getPartyName();
                    QString partyAddress = loginWindow.getPartyAddress();
                    QString partyCity = loginWindow.getPartyCity();
                    QString partyState = loginWindow.getPartyState();
                    QString partyCountry = loginWindow.getPartyCountry();

                    auto *newOrderMenu = new OrderMenu(nullptr);
                    newOrderMenu->setAttribute(Qt::WA_DeleteOnClose);
                    newOrderMenu->setInitialInfo(userName, userId,
                                                 partyName, partyId,
                                                 partyAddress, partyCity,
                                                 partyState, partyCountry);
                    newOrderMenu->insertDummyOrder();
                    newOrderMenu->show();
                    QTimer::singleShot(0, newOrderMenu, [newOrderMenu]() {
                        newOrderMenu->raise();
                        newOrderMenu->activateWindow();
                    });

                } else if (action == "orderList" &&
                           (role == "designer" || role == "manufacturer" ||
                            role == "accountant" || role == "manager" ||
                            role == "seller"))
                {
                    auto *newOrderList = new OrderList(nullptr, role);
                    newOrderList->setAttribute(Qt::WA_DeleteOnClose);
                    newOrderList->setRoleAndUserInfo(role, userId, userName);
                    newOrderList->show();
                    QTimer::singleShot(0, newOrderList, [newOrderList]() {
                        newOrderList->raise();
                        newOrderList->activateWindow();
                    });

                } else {
                    QMessageBox::warning(this, "Unknown Role", "This role is not supported.");
                }
            });

    loginWindow.exec();
}

void MainWindow::setRandomBackground()
{
    QStringList imagePaths = {
        ":/Backgrounds/1.jpg", ":/Backgrounds/2.jpg", ":/Backgrounds/3.jpg",
        ":/Backgrounds/4.jpg", ":/Backgrounds/6.jpg", ":/Backgrounds/7.jpg",
        ":/Backgrounds/8.jpg", ":/Backgrounds/9.jpg", ":/Backgrounds/10.jpg",
        ":/Backgrounds/11.jpg", ":/Backgrounds/12.jpg", ":/Backgrounds/13.jpg",
        ":/Backgrounds/14.jpg", ":/Backgrounds/15.jpg", ":/Backgrounds/16.jpg",
        ":/Backgrounds/17.jpg", ":/Backgrounds/18.jpg", ":/Backgrounds/19.jpg",
        ":/Backgrounds/20.jpg", ":/Backgrounds/21.jpg", ":/Backgrounds/22.jpg",
        ":/Backgrounds/23.jpg", ":/Backgrounds/24.jpg", ":/Backgrounds/25.jpg",
        ":/Backgrounds/26.jpg", ":/Backgrounds/27.jpg", ":/Backgrounds/28.jpg",
        ":/Backgrounds/29.jpg", ":/Backgrounds/0.jpg",  ":/Backgrounds/5.jpg"

    };

    int randomIndex = QRandomGenerator::global()->bounded(1, 30);
    currentBackground.load(imagePaths[randomIndex]);
    // currentBackground.load(":/Backgrounds/sle.jpg");
    updateBackground();  // apply scaled background
}

void MainWindow::updateBackground()
{
    if (!currentBackground.isNull()) {
        QSize labelSize = ui->backgroundLabel->size();
        QPixmap scaled = currentBackground.scaled(
            labelSize,
            Qt::KeepAspectRatioByExpanding,  // or Qt::KeepAspectRatio
            Qt::SmoothTransformation         // ensures quality scaling
            );
        ui->backgroundLabel->setPixmap(scaled);
        ui->backgroundLabel->setAlignment(Qt::AlignCenter);
        // qDebug() << "Label size at time of scaling:" << labelSize;

    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateBackground();  // Rescale background
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    updateBackground();
}


