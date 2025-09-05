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
    QString getPartyName() const { return partyName; }
    QString getPartyId() const { return partyId; }
    QString getPartyAddress() const { return partyAddress; }
    QString getPartyCity() const { return partyCity; }
    QString getPartyState() const { return partyState; }
    QString getPartyCountry() const { return partyCountry; }
    QString getRole() const { return role; }



private slots:
    void on_loginPushButton_clicked();

    void on_savePartyButton_clicked();

    void on_addPartyPushButton_clicked();

    void on_goPushButton_clicked();

    void on_backPartyPushButton_clicked();

    // void on_backPushButton_clicked();

    void set_comboBox_selectParty();

    void on_orderListPushButton_clicked();

signals:
    void loginAccepted(const QString &action);

private:
    Ui::LoginWindow *ui;
    QString userName;
    QString userId;
    QString partyName;
    QString partyId;
    QString partyAddress;
    QString partyCity;
    QString partyState;
    QString partyCountry;
    QString role;

};

#endif // LOGINWINDOW_H
