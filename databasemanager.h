#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

struct ImageRecord {
    int imageId;
    QString imagePath;
    QString imageType;
    QString designNo;
    QString companyName;
    QString goldJson;
    QString diamondJson;
    QString stoneJson;
    QString time;
    QString note;
};

class DatabaseManager {
public:
    DatabaseManager(const QString& dbName = "database/mega_mine_image.db");
    ~DatabaseManager();

    // Jewelry item operations
    bool addItem(const ImageRecord& item);
    QList<ImageRecord> getAllItems();
    ImageRecord getItemById(int id);
    bool updateItem(int id, const ImageRecord& item);
    bool deleteItem(int id);

    // Cart operations
    bool addToCart(int itemId, int quantity);
    QList<QPair<ImageRecord, int>> getCartItems();
    bool removeFromCart(int itemId);
    bool clearCart();

    // Diamond weight lookup
    double getDiamondWeight(const QString& type, const QString& size);

private:
    QSqlDatabase db;
    bool createTables();
};

#endif // DATABASEMANAGER_H
