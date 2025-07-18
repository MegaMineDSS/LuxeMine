#include "adminmenubuttons.h"
#include "ui_adminmenubuttons.h"

AdminMenuButtons::AdminMenuButtons(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdminMenuButtons)
{
    ui->setupUi(this);

    // Connect all buttons to emit and auto-close
    connect(ui->show_images, &QPushButton::clicked, this, [=]() {
        emit showImagesClicked();
        this->hide();
    });

    connect(ui->update_price, &QPushButton::clicked, this, [=]() {
        emit updatePriceClicked();
        this->hide();
    });

    connect(ui->add_dia, &QPushButton::clicked, this, [=]() {
        emit addDiamondClicked();
        this->hide();
    });

    connect(ui->orderBookUsersPushButton, &QPushButton::clicked, this, [=]() {
        emit orderBookUsersPushButtonClicked();
        this->hide();
    });

    connect(ui->orderBookRequestPushButton, &QPushButton::clicked, this, [=](){
        emit orderBookRequestPushButtonClicked();
        this->hide();
    });

    connect(ui->show_users, &QPushButton::clicked, this, [=]() {
        emit showUsersClicked();
        this->hide();
    });

    connect(ui->jewelry_menu_button, &QPushButton::clicked, this, [=]() {
        emit jewelryMenuClicked();
        this->hide();
    });

    connect(ui->logout, &QPushButton::clicked, this, [=]() {
        emit logoutClicked();
        this->hide();
    });




    this->setFixedSize(800, 600);
}

void AdminMenuButtons::hideEvent(QHideEvent *event)
{
    emit menuHidden();
    QDialog::hideEvent(event);
}

AdminMenuButtons::~AdminMenuButtons()
{
    delete ui;
}

