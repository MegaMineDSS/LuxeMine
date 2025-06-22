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
    : QDialog(parent)
    , ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::on_createAccountPushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void LoginWindow::on_savePushButton_clicked()
{
    QString userId = ui->setUserIdLineEdit->text().trimmed();
    QString userName = ui->setUserNameLineEdit->text().trimmed();
    QString password = ui->setPasswordLineEdit->text();
    QString confirmPassword = ui->confirmPasswordLineEdit->text();
    QString role = ui->setRoleLineEdit->text().trimmed();
    QString date = QDate::currentDate().toString("yyyy-MM-dd");

    // 1. Validation
    if (userId.isEmpty() || userName.isEmpty() || password.isEmpty() || confirmPassword.isEmpty() || role.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "All fields are required.");
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "Password Error", "Passwords do not match.");
        return;
    }

    // 2. Connect to DB
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    qDebug() << "Current Path:" << QDir::currentPath();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database/mega_mine_authentication.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error", "Failed to connect to database.");
        return;
    }

    // 3. Check for duplicate userId
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT userId FROM Login_DB WHERE userId = :userId");
    checkQuery.bindValue(":userId", userId);

    if (!checkQuery.exec()) {
        QMessageBox::critical(this, "Error", "Failed to check existing user: " + checkQuery.lastError().text());
        return;
    }

    if (checkQuery.next()) {
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

    if (!query.exec()) {
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
    ui->setRoleLineEdit->clear();
}

void LoginWindow::on_loginPushButton_clicked()
{
    QString userId = ui->userIdLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();

    if (userId.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login Failed", "Please enter both User ID and Password.");
        return;
    }

    // Connect to database
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database/mega_mine_authentication.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error", "Failed to connect to the database.");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT userName, role FROM Login_DB WHERE userId = :id AND password = :pwd");
    query.bindValue(":id", userId);
    query.bindValue(":pwd", password);

    if (!query.exec()) {
        QMessageBox::critical(this, "Query Error", query.lastError().text());
        return;
    }

    if (query.next()) {
        QString userName = query.value("userName").toString();
        QString role = query.value("role").toString();

        if (role == "seller") {

            this->userName = userName;  // store in member
            this->userId = userId;      // store in member
            this->accept();

        } else {
            QMessageBox::information(this, "Info", "Role not supported yet.");
        }
    } else {
        QMessageBox::warning(this, "Login Failed", "Invalid User ID or Password.");
    }
}


