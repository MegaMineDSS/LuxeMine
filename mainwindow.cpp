#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Utils.h"
#include <QRandomGenerator>
#include "user.h"
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , newAdmin(nullptr)
    , newAddCatalog(nullptr)
    , loginWindow(nullptr)
    , newUser(nullptr)
    , newOrderMenu(nullptr)
    , newOrderList(nullptr)
{
    ui->setupUi(this);

    // setWindowSize(this);

    // Set random background
    setRandomBackground();

    // Setup hover animations
    // setupAnimations();

    window()->setWindowTitle("Home");
    window()->setWindowIcon(QIcon(":/icon/homeIcon.png"));
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
        newAdmin->show();
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    if(newUser && !newUser->isHidden()){
        newUser->raise();
        newUser->activateWindow();
    }
    else{
        newUser = new User();
        newUser->show();
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if(newAddCatalog && !newAddCatalog->isHidden()){
        newAddCatalog->raise();
        newAddCatalog->activateWindow();
    }
    else{
        newAddCatalog = new AddCatalog();
        newAddCatalog->show();
    }
}

void MainWindow::on_orderBookButton_clicked()
{

    // loginWindow = new LoginWindow(this);

    // loginWindow = new LoginWindow(this);
    // if (loginWindow->exec() == QDialog::Accepted) {
    //     QString userName = loginWindow->getUserName();
    //     QString userId = loginWindow->getUserId();

    //     QString partyName = loginWindow->getPartyName();
    //     QString partyAddress = loginWindow->getPartyAddress();
    //     QString partyCity = loginWindow->getPartyCity();
    //     QString partyState = loginWindow->getPartyState();
    //     QString partyCountry = loginWindow->getPartyCountry();

    //     newOrderMenu = new OrderMenu(nullptr);
    //     newOrderMenu->setInitialInfo(userName, userId,
    //                                  partyName, partyAddress, partyCity, partyState, partyCountry);

    //     newOrderMenu->insertDummyOrder();
    //     newOrderMenu->show();
    // }

    loginWindow = new LoginWindow(this);

    if (loginWindow->exec() == QDialog::Accepted) {
        QString role = loginWindow->getRole().toLower();
        QString userName = loginWindow->getUserName();
        QString userId = loginWindow->getUserId();
        qDebug()<<role<<"---------------";

        if (role == "seller" || role == "Seller") {
            QString partyName = loginWindow->getPartyName();
            QString partyAddress = loginWindow->getPartyAddress();
            QString partyCity = loginWindow->getPartyCity();
            QString partyState = loginWindow->getPartyState();
            QString partyCountry = loginWindow->getPartyCountry();

            newOrderMenu = new OrderMenu(nullptr);
            newOrderMenu->setInitialInfo(userName, userId,
                                         partyName, partyAddress, partyCity, partyState, partyCountry);
            newOrderMenu->insertDummyOrder();
            newOrderMenu->show();
        } else if (role == "designer" || role == "manufacturer" || role == "accountant") {
            newOrderList = new OrderList();
            newOrderList->setAttribute(Qt::WA_DeleteOnClose);
            newOrderList->show();
        } else {
            QMessageBox::warning(this, "Unknown Role", "This role is not supported.");
        }
    }


}




void MainWindow::setRandomBackground()
{
    // List of image paths from the .qrc file
    QStringList imagePaths = {
        // ":/Backgrounds/0.jpg",
        ":/Backgrounds/1.jpg",
        ":/Backgrounds/2.jpg",
        ":/Backgrounds/3.jpg",
        ":/Backgrounds/4.jpg",
        // ":/Backgrounds/5.jpg",
        ":/Backgrounds/6.jpg",
        ":/Backgrounds/7.jpg",
        ":/Backgrounds/8.jpg",
        ":/Backgrounds/9.jpg",
        ":/Backgrounds/10.jpg",
        ":/Backgrounds/11.jpg",
        ":/Backgrounds/12.jpg",
        ":/Backgrounds/13.jpg",
        ":/Backgrounds/14.jpg",
        ":/Backgrounds/15.jpg",
        ":/Backgrounds/16.jpg",
        ":/Backgrounds/17.jpg",
        ":/Backgrounds/18.jpg",
        ":/Backgrounds/19.jpg",
        ":/Backgrounds/20.jpg",
        ":/Backgrounds/21.jpg",
        ":/Backgrounds/22.jpg",
        ":/Backgrounds/23.jpg",
        ":/Backgrounds/24.jpg",
        ":/Backgrounds/25.jpg",
        ":/Backgrounds/26.jpg",
        ":/Backgrounds/27.jpg",
        ":/Backgrounds/28.jpg",
        ":/Backgrounds/29.jpg",

    };

    // Generate a random index (0 to 29)
    int randomIndex = QRandomGenerator::global()->bounded(27); // 0 to 29 eccept 0 and 5

    // Set the random image as the background
    QPixmap background(imagePaths[randomIndex]);
    // ui->backgroundLabel->setPixmap(background.scaled(ui->backgroundLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    // QPixmap scaledPixmap = background.scaled(ui->backgroundLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // ui->backgroundLabel->setPixmap(scaledPixmap);
    // // Center the image if it doesn't fill the label
    // ui->backgroundLabel->setAlignment(Qt::AlignCenter);

    ui->backgroundLabel->setPixmap(background); // No scaling
    ui->backgroundLabel->setAlignment(Qt::AlignCenter);

}





