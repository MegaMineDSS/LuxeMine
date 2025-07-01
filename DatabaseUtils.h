#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QString>
#include <QStringList>
#include <QJsonArray>
#include <QMap>
#include <QSqlTableModel>
#include <QPixmap>
#include <QTableWidget>
#include "CommonTypes.h"


class DatabaseUtils
{
public:
    DatabaseUtils();

    static QStringList fetchShapes(const QString &tableType);
    static QStringList fetchSizes(const QString &tableType, const QString &shape);
    static QString saveImage(const QString &imagePath);
    static bool insertCatalogData(const QString &imagePath, const QString &imageType, const QString &designNo,
                                  const QString &companyName, const QJsonArray &goldArray, const QJsonArray &diamondArray,
                                  const QJsonArray &stoneArray, const QString &note);

    static QStringList fetchImagePaths();
    static QMap<QString, QString> fetchGoldPrices();
    static bool sizeMMExists(const QString &table, double sizeMM);
    static bool insertRoundDiamond(const QString &sieve, double sizeMM, double weight, double price);
    static bool insertFancyDiamond(const QString &shape, const QString &sizeMM, double weight, double price);
    static QSqlTableModel* createTableModel(QObject *parent, const QString &table);
    static bool updateGoldPrices(const QMap<QString, QString> &priceUpdates);

    static QJsonObject parseGoldJson(const QString &goldJson);
    static QJsonArray parseJsonArray(const QString &json);
    static bool userExists(const QString &userId);
    static bool userExistsByMobileAndName(const QString &userId, const QString &name);
    static bool insertUser(const QString &userId, const QString &companyName, const QString &mobileNo,
                           const QString &gstNo, const QString &name, const QString &emailId, const QString &address);
    static QPair<QString, QString> fetchDiamondDetails(int imageId);
    static QPair<QString, QString> fetchStoneDetails(int imageId);
    static QString fetchJsonData(int imageId, const QString &column);
    static QPixmap fetchImagePixmap(int imageId);
    static double calculateTotalGoldWeight(const QList<SelectionData> &selections);
    static void updateSummaryTable(QTableWidget *table, const QList<SelectionData> &selections, const QString &type);
    static QList<SelectionData> loadUserCart(const QString &userId);
    static bool saveUserCart(const QString &userId, const QList<SelectionData> &selections);

    static QList<QVariantList> fetchUserDetailsForAdmin();

    static bool deleteUser(const QString &userId);
    static QList<PdfRecord> getUserPdfs(const QString &userId); // Updated to return PdfRecord

    // New methods for jewelry menu
    static QList<QVariantList> fetchJewelryMenuItems();
    static bool insertJewelryMenuItem(int parentId, const QString &name, const QString &displayText);
    static bool deleteJewelryMenuItem(int id);

    static QList<ImageRecord> getAllItems();


    //ordermenu connections
    static int insertDummyOrder(const QString &sellerName, const QString &sellerId, const QString &partyName);
    static bool updateDummyOrder(int orderId, const QString &jobNo, const QString &orderNo);

    static int getNextJobNumber();
    static int getNextOrderNumberForSeller(const QString &sellerId);

    static bool saveOrder(const OrderData &order);





};

#endif // DATABASEUTILS_H
