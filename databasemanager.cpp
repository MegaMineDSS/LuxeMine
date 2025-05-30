#include "databasemanager.h"
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

DatabaseManager::DatabaseManager(const QString& dbName) {
    // Use an absolute path
    QString absoluteDbPath = dbName;
    qDebug() << "Attempting to open database at:" << absoluteDbPath;
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(absoluteDbPath);

    // Ensure the directory exists
    QDir dir = QDir(absoluteDbPath).absolutePath();
    dir.cdUp(); // Move to the parent directory
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "Error: Could not create directory:" << dir.path();
            return;
        }
    }

    if (!db.open()) {
        qDebug() << "Error: Could not open database:" << db.lastError().text();
        return;
    }

    createTables();
}

DatabaseManager::~DatabaseManager() {
    db.close();
}

bool DatabaseManager::createTables() {
    QSqlQuery query;
    // Create image_data table
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS image_data ("
        "image_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "image_path TEXT NOT NULL,"
        "image_type TEXT NOT NULL,"
        "design_no TEXT NOT NULL,"
        "company_name TEXT NOT NULL,"
        "gold_weight TEXT NOT NULL,"
        "diamond TEXT,"
        "stone TEXT,"
        "time TEXT NOT NULL,"
        "note TEXT)"
        );
    if (!success) {
        qDebug() << "Error creating image_data table:" << query.lastError().text();
        return false;
    }

    // Create Round_diamond table
    success = query.exec(
        "CREATE TABLE IF NOT EXISTS Round_diamond ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "size TEXT NOT NULL,"
        "weight REAL NOT NULL)"
        );
    if (!success) {
        qDebug() << "Error creating Round_diamond table:" << query.lastError().text();
        return false;
    }

    // Create Fancy_diamond table
    success = query.exec(
        "CREATE TABLE IF NOT EXISTS Fancy_diamond ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "size TEXT NOT NULL,"
        "weight REAL NOT NULL)"
        );
    if (!success) {
        qDebug() << "Error creating Fancy_diamond table:" << query.lastError().text();
        return false;
    }

    // Create cart table
    success = query.exec(
        "CREATE TABLE IF NOT EXISTS cart ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "item_id INTEGER NOT NULL,"
        "quantity INTEGER NOT NULL,"
        "FOREIGN KEY (item_id) REFERENCES image_data(image_id))"
        );
    if (!success) {
        qDebug() << "Error creating cart table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::addItem(const ImageRecord& item) {
    QSqlQuery query;
    query.prepare(
        "INSERT INTO image_data (image_path, image_type, design_no, company_name, gold_weight, diamond, stone, time, note) "
        "VALUES (:image_path, :image_type, :design_no, :company_name, :gold_weight, :diamond, :stone, :time, :note)"
        );
    query.bindValue(":image_path", item.imagePath);
    query.bindValue(":image_type", item.imageType);
    query.bindValue(":design_no", item.designNo);
    query.bindValue(":company_name", item.companyName);
    query.bindValue(":gold_weight", item.goldJson);
    query.bindValue(":diamond", item.diamondJson);
    query.bindValue(":stone", item.stoneJson);
    query.bindValue(":time", item.time);
    query.bindValue(":note", item.note.isEmpty() ? QVariant(QVariant::String) : item.note);

    if (!query.exec()) {
        qDebug() << "Error adding item:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<ImageRecord> DatabaseManager::getAllItems() {
    QList<ImageRecord> items;
    QSqlQuery query("SELECT image_id, image_path, image_type, design_no, company_name, gold_weight, diamond, stone, time, note FROM image_data");

    while (query.next()) {
        ImageRecord record;
        record.imageId = query.value(0).toInt();
        record.imagePath = query.value(1).toString();
        record.imageType = query.value(2).toString();
        record.designNo = query.value(3).toString();
        record.companyName = query.value(4).toString();
        record.goldJson = query.value(5).toString();
        record.diamondJson = query.value(6).toString();
        record.stoneJson = query.value(7).toString();
        record.time = query.value(8).toString();
        record.note = query.value(9).toString();
        items.append(record);
    }

    return items;
}

ImageRecord DatabaseManager::getItemById(int id) {
    QSqlQuery query;
    query.prepare("SELECT image_id, image_path, image_type, design_no, company_name, gold_weight, diamond, stone, time, note FROM image_data WHERE image_id = :id");
    query.bindValue(":id", id);
    query.exec();

    if (query.next()) {
        ImageRecord record;
        record.imageId = query.value(0).toInt();
        record.imagePath = query.value(1).toString();
        record.imageType = query.value(2).toString();
        record.designNo = query.value(3).toString();
        record.companyName = query.value(4).toString();
        record.goldJson = query.value(5).toString();
        record.diamondJson = query.value(6).toString();
        record.stoneJson = query.value(7).toString();
        record.time = query.value(8).toString();
        record.note = query.value(9).toString();
        return record;
    }
    return ImageRecord(); // Return default if not found
}

bool DatabaseManager::updateItem(int id, const ImageRecord& item) {
    QSqlQuery query;
    query.prepare(
        "UPDATE image_data SET image_path = :image_path, image_type = :image_type, design_no = :design_no, "
        "company_name = :company_name, gold_weight = :gold_weight, diamond = :diamond, stone = :stone, "
        "time = :time, note = :note WHERE image_id = :id"
        );
    query.bindValue(":image_path", item.imagePath);
    query.bindValue(":image_type", item.imageType);
    query.bindValue(":design_no", item.designNo);
    query.bindValue(":company_name", item.companyName);
    query.bindValue(":gold_weight", item.goldJson);
    query.bindValue(":diamond", item.diamondJson);
    query.bindValue(":stone", item.stoneJson);
    query.bindValue(":time", item.time);
    query.bindValue(":note", item.note.isEmpty() ? QVariant(QVariant::String) : item.note);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Error updating item:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteItem(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM image_data WHERE image_id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Error deleting item:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addToCart(int itemId, int quantity) {
    QSqlQuery query;
    query.prepare("INSERT INTO cart (item_id, quantity) VALUES (:item_id, :quantity)");
    query.bindValue(":item_id", itemId);
    query.bindValue(":quantity", quantity);

    if (!query.exec()) {
        qDebug() << "Error adding to cart:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<QPair<ImageRecord, int>> DatabaseManager::getCartItems() {
    QList<QPair<ImageRecord, int>> cartItems;
    QSqlQuery query("SELECT i.image_id, i.image_path, i.image_type, i.design_no, i.company_name, i.gold_weight, i.diamond, i.stone, i.time, i.note, c.quantity "
                    "FROM cart c JOIN image_data i ON c.item_id = i.image_id");

    while (query.next()) {
        ImageRecord record;
        record.imageId = query.value(0).toInt();
        record.imagePath = query.value(1).toString();
        record.imageType = query.value(2).toString();
        record.designNo = query.value(3).toString();
        record.companyName = query.value(4).toString();
        record.goldJson = query.value(5).toString();
        record.diamondJson = query.value(6).toString();
        record.stoneJson = query.value(7).toString();
        record.time = query.value(8).toString();
        record.note = query.value(9).toString();
        int quantity = query.value(10).toInt();
        cartItems.append(qMakePair(record, quantity));
    }

    return cartItems;
}

bool DatabaseManager::removeFromCart(int itemId) {
    QSqlQuery query;
    query.prepare("DELETE FROM cart WHERE item_id = :item_id");
    query.bindValue(":item_id", itemId);

    if (!query.exec()) {
        qDebug() << "Error removing from cart:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::clearCart() {
    QSqlQuery query("DELETE FROM cart");
    if (!query.exec()) {
        qDebug() << "Error clearing cart:" << query.lastError().text();
        return false;
    }
    return true;
}

double DatabaseManager::getDiamondWeight(const QString& type, const QString& size) {
    QSqlQuery query;
    QString tableName = (type.toLower() == "round") ? "Round_diamond" : "Fancy_diamond";
    query.prepare(QString("SELECT weight FROM %1 WHERE sizeMM = :size").arg(tableName));
    query.bindValue(":size", size);

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    } else {
        qDebug() << "Error or no weight found for type:" << type << "size:" << size << query.lastError().text();
        return 0.0; // Default to 0 if not found
    }
}
