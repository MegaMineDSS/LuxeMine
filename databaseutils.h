#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QJsonArray>
#include <QMap>
#include <QPixmap>
#include <QSqlTableModel>
#include <QString>
#include <QStringList>
#include <QTableWidget>

#include <xlsxdocument.h>
#include <xlsxworksheet.h>

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
        // static bool updateRoleStatus(const QString &jobNo, const QString &fieldName, const QString &newStatus);
        static bool updateRoleStatus(const QString &jobNo, const QString &role, const QString &newStatus);
        static QList<JobSheetRequest> fetchJobSheetRequests();


    //User connection
        // User-related operations
        static bool userExists(const QString &userId);
        static bool userExistsByMobileAndName(const QString &userId, const QString &name);
        static bool insertUser(const QString &userId, const QString &companyName, const QString &mobileNo,
                               const QString &gstNo, const QString &name, const QString &emailId, const QString &address);

        // Cart operations
        static QList<SelectionData> loadUserCart(const QString &userId);
        static bool saveUserCart(const QString &userId, const QList<SelectionData> &selections);

        // Item / Image operations
        static QList<ImageRecord> getAllItems();
        static QPixmap fetchImagePixmap(int imageId);
        static QString fetchJsonData(int imageId, const QString &column);

        // Details (Diamond, Stone, Gold)
        static QPair<QString, QString> fetchDiamondDetails(int imageId);
        static QPair<QString, QString> fetchStoneDetails(int imageId);
        static double calculateTotalGoldWeight(const QList<SelectionData> &selections);
        static QJsonObject parseGoldJson(const QString &goldJson);

        // JSON utilities
        static QJsonArray parseJsonArray(const QString &json);

        // UI / Summary Helpers
        static void updateSummaryTable(QTableWidget *table, const QList<SelectionData> &selections, const QString &type);


    //Add Catalog Connection
        //All operation
        static QStringList fetchShapes(const QString &tableType);
        static QStringList fetchSizes(const QString &tableType, const QString &shape);
        static QString saveImage(const QString &imagePath);
        static QString insertCatalogData(const QString &imagePath, const QString &imageType, const QString &designNo,
                                      const QString &companyName, const QJsonArray &goldArray, const QJsonArray &diamondArray,
                                      const QJsonArray &stoneArray, const QString &note);
        static QJsonArray generateGoldWeights(int inputKarat, double inputWeight) ;
        static QJsonArray buildDiamondArray(const QList<QVariantMap> &diamondRows, const QString &designNo) ;

        static QJsonArray buildStoneArray(const QList<QVariantMap> &stoneRows, const QString &designNo);
        static bool excelBulkInsertCatalog(const QString &filePath);


    // Login Window
        // Authentication
        static LoginResult authenticateUser(const QString &userId, const QString &password);
        static bool userLoginValidate(const QString &userId, const QString &passwd);
        // Party Management
        static QStringList fetchPartyNamesForUser(const QString &userId);
        static bool insertParty(const PartyData &party);
        static PartyInfo fetchPartyDetails(const QString &userId, const QString &partyId);


    // OrderMenu Connections
        // Order Initialization
        static int insertDummyOrder(const QString &sellerName, const QString &sellerId, const QString &partyName);
        static int getNextJobNumber();
        static int getNextOrderNumberForSeller(const QString &sellerId);

        // Order Updates
        static bool updateDummyOrder(int orderId, const QString &jobNo, const QString &orderNo);
        static bool cleanupUnsavedOrders();

        // Save Order
        static bool saveOrder(const OrderData &order);


    // OrderList Connections
        // Job Sheet Data
        static std::optional<JobSheetData> fetchJobSheetData(const QString &jobNo);
        static QPair<QString, QString> fetchDiamondAndStoneJson(const QString &designNo);

        // Status Change Requests
        static bool insertStatusChangeRequest(const QString &jobNo, const QString &userId,
                                              const QString &fromStatus, const QString &toStatus,
                                              const QString &role, const QString &note);
        static bool approveStatusChange(const QString &jobNo, const QString &role,
                                        const QString &statusField, bool approved,
                                        const QString &note);

        // Order List
        static QList<QVariantList> fetchOrderListDetails();

    // JobSheet Connections
        // Image / Design
        static QString fetchImagePathForDesign(const QString &designNo);
        static bool updateDesignNoAndImagePath(const QString &jobNo, const QString &designNo, const QString &imagePath);

        // Stone Details
        static void fillStoneTable(QTableWidget *table, const QString &designNo);


};

#endif // DATABASEUTILS_H
