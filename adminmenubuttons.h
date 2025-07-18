#ifndef ADMINMENUBUTTONS_H
#define ADMINMENUBUTTONS_H

#include <QDialog>


namespace Ui {
class AdminMenuButtons;
}

class AdminMenuButtons : public QDialog
{
    Q_OBJECT

public:
    explicit AdminMenuButtons(QWidget *parent = nullptr);
    ~AdminMenuButtons();


signals:
    void showImagesClicked();
    void updatePriceClicked();
    void addDiamondClicked();
    void showUsersClicked();
    void jewelryMenuClicked();
    void logoutClicked();
    void menuHidden();  ///< Emitted when the menu hides

    void orderBookUsersPushButtonClicked();
    void orderBookRequestPushButtonClicked();


private:
    void hideEvent(QHideEvent *event);

private:
    Ui::AdminMenuButtons *ui;
};

#endif // ADMINMENUBUTTONS_H
