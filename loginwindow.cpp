#include "loginwindow.h"
#include "ui_loginwindow.h"

#include <QMessageBox>
#include <QDate>
#include <QDir>
#include <QDebug>

#include "DatabaseUtils.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::set_comboBox_selectParty()
{
    QStringList parties = DatabaseUtils::fetchPartyNamesForUser(userId);
    ui->selectPartyCombobox->clear();
    ui->selectPartyCombobox->addItems(parties);
}

void LoginWindow::on_savePartyButton_clicked()
{
    PartyData party;
    party.name = ui->partyNameLineEdit->text().trimmed();
    party.id = ui->setPartyIdLineEdit->text().trimmed();
    party.email = ui->partyEmailLineEdit->text().trimmed();
    party.mobileNo = ui->partyMobileNoLineEdit->text().toInt();
    party.address = ui->partyAddressTextEdit->toPlainText().trimmed();
    party.city = ui->partyCityLineEdit->text().trimmed();
    party.state = ui->partyStateLineEdit->text().trimmed();
    party.country = ui->partyCountryLineEdit->text().trimmed();
    party.areaCode = ui->partyAreaCodeLineEdit->text().toInt();
    party.userId = userId;
    party.date = QDate::currentDate().toString("yyyy-MM-dd");

    if (party.id.isEmpty() || party.name.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "All fields are required.");
        return;
    }

    if (!DatabaseUtils::insertParty(party)) {
        QMessageBox::critical(this, "Database Error", "Failed to save party.");
        return;
    }

    QMessageBox::information(this, "Success", "Party Added!");
    ui->stackedWidget->setCurrentIndex(1);
    set_comboBox_selectParty();

    ui->partyNameLineEdit->clear();
    ui->setPartyIdLineEdit->clear();
    ui->partyMobileNoLineEdit->clear();
    ui->partyEmailLineEdit->clear();
    ui->partyAddressTextEdit->clear();
    ui->partyCityLineEdit->clear();
    ui->partyStateLineEdit->clear();
    ui->partyCountryLineEdit->clear();
    ui->partyAreaCodeLineEdit->clear();
}

void LoginWindow::on_addPartyPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void LoginWindow::on_goPushButton_clicked()
{
    QString partyText = ui->selectPartyCombobox->currentText();  // e.g., "Client (12)"
    if (partyText == "-") {
        QMessageBox::warning(this, "Not Valid", "Choose a party name");
        return;
    }

    QRegularExpression regex("^(.*) \\((.*)\\)$");
    QRegularExpressionMatch match = regex.match(partyText);

    if (!match.hasMatch()) {
        qDebug() << "Invalid combo box text format:" << partyText;
        return;
    }

    QString name = match.captured(1).trimmed();
    QString idStr = match.captured(2).trimmed();

    PartyInfo info = DatabaseUtils::fetchPartyDetails(userId, idStr);

    if (info.id.isEmpty()) {
        QMessageBox::critical(this, "Error", "Failed to load party details.");
        return;
    }

    this->partyId = info.id;
    this->partyName = info.name;
    this->partyAddress = info.address;
    this->partyCity = info.city;
    this->partyState = info.state;
    this->partyCountry = info.country;

    this->accept();
}


void LoginWindow::on_backPartyPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    set_comboBox_selectParty();
}

void LoginWindow::on_backPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void LoginWindow::on_loginPushButton_clicked()
{
    userId = ui->userIdLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();

    if (userId.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login Failed", "Please enter both User ID and Password.");
        return;
    }

    LoginResult result = DatabaseUtils::authenticateUser(userId, password);

    if (!result.success) {
        QMessageBox::warning(this, "Login Failed", "Invalid User ID or Password.");
        return;
    }

    this->role = result.role;
    this->userName = result.userName;
    this->userId = userId;

    if (role.compare("Seller", Qt::CaseInsensitive) == 0) {
        ui->stackedWidget->setCurrentIndex(1);
        set_comboBox_selectParty();
    } else if (role == "Designer" || role == "Manufacturer" || role == "Accountant") {
        this->accept(); // Valid user, return control to main
    } else {
        QMessageBox::information(this, "Info", "Role not supported yet.");
    }
}
