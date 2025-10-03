#ifndef USER_H
#define USER_H

#include <QDialog>

#include <QJsonObject>

#include "commontypes.h"

class QTableWidget;

namespace Ui {
class User;
}

class User : public QDialog
{
    Q_OBJECT

public:
    explicit User(QWidget *parent = nullptr);
    ~User();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_user_registration_button_clicked();
    void on_previousImage_clicked();
    void on_nextImage_clicked();
    void on_cartMainpage_clicked();
    void on_backMainpage_clicked();
    void on_cartItemQuantityChanged(int imageId, const QString &goldType, int newQuantity);
    void on_cartItemRemoveRequested(int imageId, const QString &goldType);
    void on_makePdfButton_clicked();


    void on_make_pdf_Mainpage_clicked();

    void on_user_register_redirect_button_clicked();

    void on_user_login_button_clicked();


    void on_registration_verify_checkbox_clicked();

    void on_resigter_page_login_button_clicked();


public slots:
    void loadImage(int index);
    void updateGoldWeight(const QString &goldJson);
    void on_selectButton_clicked();

private:
    void setupUi();
    void setupMobileComboBox();
    void loadData();
    void displayDiamondDetails();
    void displayStoneDetails();
    void updateCartDisplay();
    void updateGoldSummary();
    void updateDiamondSummary();
    void updateStoneSummary();
    bool saveOrLoadUser();
    bool handleRegistration();
    bool canRegister(const QString &mobilePrefix, const QString &mobileNo);
    void loadUserCart(const QString &userId);
    void saveCartToDatabase();
    void selectMobileCodeFromText(const QString &text);

    Ui::User *ui;
    QJsonObject goldData;
    int currentImageIndex;
    QList<ImageRecord> imageRecords;
    QTableWidget *diamondTable;
    QString currentDiamondJson;
    QTableWidget *stoneTable;
    QString currentStoneJson;
    QVector<SelectionData> selections;
    QWidget *cartItemsContainer;
    QString currentUserId;
    QString currentGoldSelection;
};

#endif // USER_H
