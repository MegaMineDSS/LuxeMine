#ifndef USER_H
#define USER_H

#include <QDialog>
#include "CommonTypes.h"

class QJsonObject;
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

private slots:
    void on_pushButton_clicked();
    void on_previousImage_clicked();
    void on_nextImage_clicked();
    void on_cartMainpage_clicked();
    void on_backMainpage_clicked();
    void on_cartItemQuantityChanged(int imageId, const QString &goldType, int newQuantity);
    void on_cartItemRemoveRequested(int imageId, const QString &goldType);
    void on_makePdfButton_clicked();

public slots:
    void loadImage(int index);
    void updateGoldWeight();
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
    void loadUserCart(const QString &userId);
    void saveCartToDatabase();

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
};

#endif // USER_H
