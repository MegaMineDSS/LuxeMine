#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    QString getUserName() const { return userName; }
    QString getUserId() const { return userId; }

private slots:
    void on_createAccountPushButton_clicked();

    void on_savePushButton_clicked();

    void on_loginPushButton_clicked();

private:
    Ui::LoginWindow *ui;
    QString userName;
    QString userId;
};

#endif // LOGINWINDOW_H
