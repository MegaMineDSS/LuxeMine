#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDir>
#include <QDebug>
#include "ordermenu.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::set_comboBox_role()
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");   // SQLite driver
    db.setDatabaseName("database/luxeMineAuthentication.db"); // Path to your SQLite file

    if (!db.open())
    {
        qDebug() << "Database connection failed:" << db.lastError().text();
    }
    else
    {
        qDebug() << "Database opened successfully!";
    }

    QSqlQuery selectRole("SELECT role FROM OrderBook_Roles");

    if (!selectRole.exec())
    {
        qDebug() << "query not executed";
        return;
    }
    ui->setRoleComboBox->clear();
    ui->setRoleComboBox->addItem("-");
    while (selectRole.next())
    {
        QString roleValue = selectRole.value(0).toString();
        ui->setRoleComboBox->addItem(roleValue);
    }
    db.close();
}

void LoginWindow::set_comboBox_selectParty()
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");   // SQLite driver
    db.setDatabaseName("database/luxeMineAuthentication.db"); // Path to your SQLite file

    if (!db.open())
    {
        qDebug() << "Database connection failed:" << db.lastError().text();
    }
    else
    {
        qDebug() << "Database opened successfully!";
    }

    QSqlQuery selectRole("SELECT name FROM Partys");

    if (!selectRole.exec())
    {
        qDebug() << "query not executed";
        return;
    }
    ui->selectPartyCombobox->clear();
    ui->selectPartyCombobox->addItem("-");
    while (selectRole.next())
    {
        QString roleValue = selectRole.value(0).toString();
        ui->selectPartyCombobox->addItem(roleValue);
    }

    db.close();
}

void LoginWindow::on_createAccountPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    set_comboBox_role();
}

void LoginWindow::on_savePartyButton_clicked()
{
    QString partyName = ui->partyNameLineEdit->text().trimmed();
    QString partyId = ui->setPartyIdLineEdit->text().trimmed();
    QString partyEmail = ui->partyEmailLineEdit->text().trimmed();
    int partyMobileNo = ui->partyMobileNoLineEdit->text().toInt();
    QString partyAddress = ui->partyAddressTextEdit->toPlainText().trimmed();
    QString partyCity = ui->partyCityLineEdit->text().trimmed();
    QString partyState = ui->partyStateLineEdit->text().trimmed();
    QString partyCountry = ui->partyCountryLineEdit->text().trimmed();
    int partyAreaCode = ui->partyAreaCodeLineEdit->text().toInt();
    QString partyDate = QDate::currentDate().toString("yyyy-MM-dd");

    if (partyId.isEmpty() || partyName.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "All fields are required.");
        return;
    }

    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "add_party");
    db.setDatabaseName("database/luxeMineAuthentication.db");

    // ❗ You must call open() first
    if (!db.open())
    {
        qDebug() << "Database not opened: " << db.lastError().text();
        QMessageBox::critical(this, "DB Error", "Failed to open database.");
        return;
    }

    // Now database is open, continue
    QSqlQuery partyAdd(db); // Pass the correct connection
    partyAdd.prepare("INSERT INTO Partys (id, name, email, mobileNo, address, city, state, country, areaCode, userId, date) "
                     "VALUES (:partyId, :partyName, :partyEmail, :partyMobileNo, :partyAddress, :partyCity, :partyState, :partyCountry, :partyAreaCode, :partyUserId, :partyDate)");

    partyAdd.bindValue(":partyId", partyId);
    partyAdd.bindValue(":partyName", partyName);
    partyAdd.bindValue(":partyEmail", partyEmail);
    partyAdd.bindValue(":partyMobileNo", partyMobileNo);
    partyAdd.bindValue(":partyAddress", partyAddress);
    partyAdd.bindValue(":partyCity", partyCity);
    partyAdd.bindValue(":partyState", partyState);
    partyAdd.bindValue(":partyCountry", partyCountry);
    partyAdd.bindValue(":partyAreaCode", partyAreaCode);
    partyAdd.bindValue(":partyUserId", userId); // ❗ Make sure this is passed
    partyAdd.bindValue(":partyDate", partyDate);

    if (!partyAdd.exec())
    {
        QMessageBox::critical(this, "Database Error", "Failed to save party: " + partyAdd.lastError().text());
        return;
    }

    QMessageBox::information(this, "Success", "Party Added!");

    ui->stackedWidget->setCurrentIndex(2);
    set_comboBox_selectParty();
    ui->partyNameLineEdit->clear();
    ui->setPartyIdLineEdit->clear();
    ui->partyMobileNoLineEdit->clear();
    ui->partyEmailLineEdit->clear();
    ui->partyAddressTextEdit->clear();
    ui->partyCityLineEdit->clear();
    ui->partyStateLineEdit->clear();
    ui->partyCountryLineEdit->clear();
    ui->partyAreaCodeLabel->clear();

    db.close();
}

void LoginWindow::on_addPartyPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
}

void LoginWindow::on_goPushButton_clicked()
{
    this->accept();
}

void LoginWindow::on_backPartyPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    set_comboBox_selectParty();
}

void LoginWindow::on_backPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void LoginWindow::on_savePushButton_clicked()
{
    QString userId = ui->setUserIdLineEdit->text().trimmed();
    QString userName = ui->setUserNameLineEdit->text().trimmed();
    QString password = ui->setPasswordLineEdit->text();
    QString confirmPassword = ui->confirmPasswordLineEdit->text();
    // QString role = ui->setRoleLineEdit->text().trimmed();
    QString role = ui->setRoleComboBox->currentText();
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    // 1. Validation
    if (userId.isEmpty() || userName.isEmpty() || password.isEmpty() || confirmPassword.isEmpty() || role.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "All fields are required.");
        return;
    }

    if (password != confirmPassword)
    {
        QMessageBox::warning(this, "Password Error", "Passwords do not match.");
        return;
    }

    // 2. Connect to DB
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    qDebug() << "Current Path:" << QDir::currentPath();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database/mega_mine_authentication.db");

    if (!db.open())
    {
        QMessageBox::critical(this, "Database Error", "Failed to connect to database.");
        return;
    }

    // 3. Check for duplicate userId
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT userId FROM Login_DB WHERE userId = :userId");
    checkQuery.bindValue(":userId", userId);

    if (!checkQuery.exec())
    {
        QMessageBox::critical(this, "Error", "Failed to check existing user: " + checkQuery.lastError().text());
        return;
    }

    if (checkQuery.next())
    {
        QMessageBox::warning(this, "Duplicate User", "User ID already exists.");
        return;
    }

    // 4. Insert new user
    QSqlQuery query;
    query.prepare("INSERT INTO Login_DB (userId, userName, password, date, role) "
                  "VALUES (:userId, :userName, :password, :date, :role)");
    query.bindValue(":userId", userId);
    query.bindValue(":userName", userName);
    query.bindValue(":password", password);
    query.bindValue(":date", date);
    query.bindValue(":role", role);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Database Error", "Failed to save user: " + query.lastError().text());
        return;
    }

    QMessageBox::information(this, "Success", "User account created successfully!");

    // Go back to login page
    ui->stackedWidget->setCurrentIndex(0);

    // Clear form
    ui->setUserIdLineEdit->clear();
    ui->setUserNameLineEdit->clear();
    ui->setPasswordLineEdit->clear();
    ui->confirmPasswordLineEdit->clear();
    // ui->setRoleLineEdit->clear();
    ui->setRoleComboBox->setCurrentIndex(0);
}

void LoginWindow::on_loginPushButton_clicked()
{
    QString userId = ui->userIdLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();

    if (userId.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "Login Failed", "Please enter both User ID and Password.");
        return;
    }

    // Connect to database
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database/mega_mine_authentication.db");

    if (!db.open())
    {
        QMessageBox::critical(this, "Database Error", "Failed to connect to the database.");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT userName, role FROM Login_DB WHERE userId = :id AND password = :pwd");
    query.bindValue(":id", userId);
    query.bindValue(":pwd", password);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        return;
    }

    if (query.next())
    {
        QString userName = query.value("userName").toString();
        QString role = query.value("role").toString();
        this->role = role;


        if (role == "Seller" || role == "seller")
        {
            this->userName = userName; // store in member
            this->userId = userId;

            // ✅ Open Partys DB
            QSqlDatabase partyDb = QSqlDatabase::addDatabase("QSQLITE", "party_db");
            partyDb.setDatabaseName("database/luxeMineAuthentication.db");

            if (!partyDb.open())
            {
                qDebug() << "Failed to open party DB:" << partyDb.lastError().text();
            }
            else
            {
                QSqlQuery partyQuery(partyDb);
                partyQuery.prepare("SELECT name, address, city, state, country FROM Partys WHERE userId = :uid LIMIT 1");
                partyQuery.bindValue(":uid", userId); // ✅ make sure name matches

                if (partyQuery.exec() && partyQuery.next())
                {
                    this->partyName = partyQuery.value("name").toString();
                    this->partyAddress = partyQuery.value("address").toString();
                    this->partyCity = partyQuery.value("city").toString();
                    this->partyState = partyQuery.value("state").toString();
                    this->partyCountry = partyQuery.value("country").toString();
                }
                else
                {
                    qDebug() << "Party query failed:" << partyQuery.lastError().text();
                }

                partyDb.close();
            }

            // store in member
            // this->accept();
            ui->stackedWidget->setCurrentIndex(2);
            set_comboBox_selectParty();
        }
        else if (role == "Designer" || role == "Manufacturer" || role == "Accountant") {
            // Success! Let mainwindow decide what to open
            this->accept(); // closes login dialog and returns QDialog::Accepted
        }
        else
        {
            QMessageBox::information(this, "Info", "Role not supported yet.");
        }
    }
    else
    {
        QMessageBox::warning(this, "Login Failed", "Invalid User ID or Password.");
    }
}
