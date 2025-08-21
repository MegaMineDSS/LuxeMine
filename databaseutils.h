#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QJsonArray>
#include <QMap>
#include <QPixmap>
#include <QSqlTableModel>
#include <QString>
#include <QStringList>
#include <QTableWidget>

#include "commontypes.h"

class DatabaseUtils
{
public:
    DatabaseUtils();

    //Admin Logic
        //Jewellry Menu
        static bool deleteJewelryMenuItem(int id);
        static bool insertJewelryMenuItem(int parentId, const QString &name, const QString &displayText);
        static QList<QVariantList> fetchJewelryMenuItems();

        //Gold
        static QMap<QString, QString> fetchGoldPrices();
        static bool updateGoldPrices(const QMap<QString, QString> &priceUpdates);

        //Diamond
        static bool sizeMMExists(const QString &table, double sizeMM);
        static bool insertRoundDiamond(const QString &sieve, double sizeMM, double weight, double price);
        static bool insertFancyDiamond(const QString &shape, const QString &sizeMM, double weight, double price);
        static QSqlTableModel* createTableModel(QObject *parent, const QString &table);

        //User Management
        static QStringList fetchRoles();
        static QStringList fetchImagePaths();
        static bool deleteUser(const QString &userId);
        static QList<QVariantList> fetchUserDetailsForAdmin();
        static QList<PdfRecord> getUserPdfs(const QString &userId); // Updated to return PdfRecord
        static bool checkAdminCredentials(const QString &username, const QString &password, QString &role);
        static bool createOrderBookUser(const QString &userId, const QString &userName,
                                        const QString &password, const QString &role, const QString &date,
                                        QString &errorMsg);

        //Job Sheet / Order Book
        static bool updateStatusChangeRequest(int requestId, bool approved, const QString &note);
        static bool updateRoleStatus(const QString &jobNo, const QString &fieldName, const QString &newStatus);
        static QList<JobSheetRequest> fetchJobSheetRequests();

    //not fixed
    static QStringList fetchShapes(const QString &tableType);
    static QStringList fetchSizes(const QString &tableType, const QString &shape);
    static QString saveImage(const QString &imagePath);
    static bool insertCatalogData(const QString &imagePath, const QString &imageType, const QString &designNo,
                                  const QString &companyName, const QJsonArray &goldArray, const QJsonArray &diamondArray,
                                  const QJsonArray &stoneArray, const QString &note);





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









    static QList<ImageRecord> getAllItems();


    //ordermenu connections
    static int insertDummyOrder(const QString &sellerName, const QString &sellerId, const QString &partyName);
    static bool updateDummyOrder(int orderId, const QString &jobNo, const QString &orderNo);

    static int getNextJobNumber();
    static int getNextOrderNumberForSeller(const QString &sellerId);

    static bool saveOrder(const OrderData &order);
    static QList<QVariantList> fetchOrderListDetails();


    //Login window connections
    static QStringList fetchPartyNamesForUser(const QString &userId);
    static bool insertParty(const PartyData &party);
    static PartyInfo fetchPartyDetails(const QString &userId, const QString &partyId);
    static LoginResult authenticateUser(const QString &userId, const QString &password);



};

#endif // DATABASEUTILS_H
