#include "DatabaseUtils.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QJsonDocument>
#include <QDateTime>
#include <QPixmap>
#include <QJsonObject>
#include <QHeaderView>
#include <QCoreApplication>

QStringList DatabaseUtils::fetchShapes(const QString &tableType)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "shapes_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return {};

    QSqlQuery query(db);
    QString queryStr = tableType == "diamond" ?
                           "SELECT DISTINCT shape FROM Fancy_diamond UNION SELECT 'Round' FROM Round_diamond" :
                           "SELECT DISTINCT shape FROM stones";

    if (!query.exec(queryStr)) {
        db.close();
        return {};
    }

    QStringList shapes;
    while (query.next()) shapes.append(query.value(0).toString());
    shapes.sort(Qt::CaseInsensitive);
    db.close();
    return shapes;
}

QStringList DatabaseUtils::fetchSizes(const QString &tableType, const QString &shape)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "sizes_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return {};

    QSqlQuery query(db);
    if (tableType == "diamond") {
        if (shape == "Round") {
            query.prepare("SELECT DISTINCT sizeMM FROM Round_diamond");
        } else {
            query.prepare("SELECT DISTINCT sizeMM FROM Fancy_diamond WHERE shape = :shape");
            query.bindValue(":shape", shape);
        }
    } else {
        query.prepare("SELECT DISTINCT sizeMM FROM stones WHERE shape = :shape");
        query.bindValue(":shape", shape);
    }

    if (!query.exec()) {
        db.close();
        return {};
    }

    QStringList sizes;
    while (query.next()) sizes.append(query.value(0).toString());
    db.close();
    return sizes;
}

QString DatabaseUtils::saveImage(const QString &imagePath)
{
    QFile imageFile(imagePath);
    if (!imageFile.exists()) return {};

    QString targetDir = QDir(QDir::currentPath()).filePath("images");
    QDir().mkpath(targetDir);
    QString newImagePath = "images/" + QFileInfo(imagePath).fileName();

    if (!QFile::copy(imagePath, newImagePath)) return {};
    return newImagePath;
}

bool DatabaseUtils::insertCatalogData(const QString &imagePath, const QString &imageType, const QString &designNo,
                                      const QString &companyName, const QJsonArray &goldArray, const QJsonArray &diamondArray,
                                      const QJsonArray &stoneArray, const QString &note)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insert_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QJsonDocument goldDoc(goldArray);
    QJsonDocument diamondDoc(diamondArray);
    QJsonDocument stoneDoc(stoneArray);

    QSqlQuery query(db);
    query.prepare("INSERT INTO image_data (image_path, image_type, design_no, company_name, gold_weight, diamond, stone, time, note) "
                  "VALUES (:image_path, :image_type, :design_no, :company_name, :gold_weight, :diamond, :stone, :time, :note)");
    query.bindValue(":image_path", imagePath);
    query.bindValue(":image_type", imageType);
    query.bindValue(":design_no", designNo);
    query.bindValue(":company_name", companyName);
    query.bindValue(":gold_weight", goldDoc.toJson(QJsonDocument::Compact));
    query.bindValue(":diamond", diamondDoc.toJson(QJsonDocument::Compact));
    query.bindValue(":stone", stoneDoc.toJson(QJsonDocument::Compact));
    query.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":note", note);

    bool success = query.exec();
    db.close();
    return success;
}

QStringList DatabaseUtils::fetchImagePaths()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "images_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return {};

    QSqlQuery query("SELECT image_path FROM image_data", db);
    if (!query.exec()) {
        db.close();
        return {};
    }

    QStringList paths;
    while (query.next()) paths.append(query.value(0).toString());
    db.close();
    return paths;
}

QMap<QString, QString> DatabaseUtils::fetchGoldPrices()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "gold_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return {};

    QSqlQuery query("SELECT Kt, Price FROM Gold_Price", db);
    if (!query.exec()) {
        db.close();
        return {};
    }

    QMap<QString, QString> prices;
    while (query.next()) prices[query.value(0).toString().trimmed()] = query.value(1).toString();
    db.close();
    return prices;
}

bool DatabaseUtils::sizeMMExists(const QString &table, double sizeMM)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "check_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM " + table + " WHERE sizeMM = :sizeMM");
    query.bindValue(":sizeMM", sizeMM);

    if (!query.exec() || !query.next()) {
        db.close();
        return false;
    }

    bool exists = query.value(0).toInt() > 0;
    db.close();
    return exists;
}

bool DatabaseUtils::insertRoundDiamond(const QString &sieve, double sizeMM, double weight, double price)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insert_round_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO Round_diamond (sieve, sizeMM, weight, price) "
                  "VALUES (:sieve, :sizeMM, :weight, :price)");
    query.bindValue(":sieve", sieve);
    query.bindValue(":sizeMM", sizeMM);
    query.bindValue(":weight", weight);
    query.bindValue(":price", price);

    bool success = query.exec();
    db.close();
    return success;
}

bool DatabaseUtils::insertFancyDiamond(const QString &shape, const QString &sizeMM, double weight, double price)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insert_fancy_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO Fancy_diamond (shape, sizeMM, weight, price) "
                  "VALUES (:shape, :sizeMM, :weight, :price)");
    query.bindValue(":shape", shape);
    query.bindValue(":sizeMM", sizeMM);
    query.bindValue(":weight", weight);
    query.bindValue(":price", price);

    bool success = query.exec();
    db.close();
    return success;
}

QSqlTableModel* DatabaseUtils::createTableModel(QObject *parent, const QString &table)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "table_model_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return nullptr;

    QSqlTableModel *model = new QSqlTableModel(parent, db);
    model->setTable(table);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    if (!model->select()) {
        delete model;
        db.close();
        return nullptr;
    }

    return model;
}

bool DatabaseUtils::updateGoldPrices(const QMap<QString, QString> &priceUpdates)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "update_gold_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    bool success = true;

    for (auto it = priceUpdates.constBegin(); it != priceUpdates.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            query.prepare("UPDATE Gold_Price SET Price = :price WHERE Kt = :kt");
            query.bindValue(":price", it.value());
            query.bindValue(":kt", it.key());
            if (!query.exec()) {
                success = false;
                break;
            }
        }
    }

    db.close();
    return success;
}

QJsonObject DatabaseUtils::parseGoldJson(const QString &goldJson)
{
    QJsonObject goldData;
    QJsonDocument doc = QJsonDocument::fromJson(goldJson.toUtf8());
    if (doc.isArray()) {
        QJsonArray goldArray = doc.array();
        for (const QJsonValue &value : goldArray) {
            QJsonObject obj = value.toObject();
            QString karat = obj["karat"].toString();
            double weight = obj["weight(g)"].toString().toDouble();
            if (!karat.isEmpty()) goldData[karat] = weight;
        }
    }
    return goldData;
}

QJsonArray DatabaseUtils::parseJsonArray(const QString &json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.isArray() ? doc.array() : QJsonArray();
}

bool DatabaseUtils::userExists(const QString &userId)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "user_check_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM users WHERE user_id = :user_id");
    query.bindValue(":user_id", userId);
    bool exists = query.exec() && query.next();
    db.close();
    return exists;
}

bool DatabaseUtils::userExistsByMobileAndName(const QString &userId, const QString &name)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "user_mobile_check_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM users WHERE mobile_no = :mobile_no AND name = :name");
    query.bindValue(":mobile_no", userId);
    query.bindValue(":name", name);
    bool exists = query.exec() && query.next();
    db.close();
    return exists;
}

bool DatabaseUtils::insertUser(const QString &userId, const QString &companyName, const QString &mobileNo,
                               const QString &gstNo, const QString &name, const QString &emailId, const QString &address)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "user_insert_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO users (user_id, company_name, mobile_no, gst_no, name, email_id, address, time) "
                  "VALUES (:user_id, :company_name, :mobile_no, :gst_no, :name, :email_id, :address, :time)");
    query.bindValue(":user_id", userId);
    query.bindValue(":company_name", companyName);
    query.bindValue(":mobile_no", mobileNo);
    query.bindValue(":gst_no", gstNo);
    query.bindValue(":name", name);
    query.bindValue(":email_id", emailId);
    query.bindValue(":address", address);
    query.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));

    bool success = query.exec();
    db.close();
    return success;
}

QPair<QString, QString> DatabaseUtils::fetchDiamondDetails(int imageId)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "diamond_details_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return {};

    QSqlQuery query(db);
    query.prepare("SELECT diamond FROM image_data WHERE image_id = :imageId");
    query.bindValue(":imageId", imageId);

    QString json, detailText;
    double totalWeight = 0.0;
    QMap<QString, double> weightByType;

    if (query.exec() && query.next()) {
        json = query.value("diamond").toString();
        QJsonArray array = parseJsonArray(json);
        for (const QJsonValue &value : array) {
            QJsonObject obj = value.toObject();
            QString type = obj["type"].toString().trimmed().toLower();
            QString sizeMM = obj["sizeMM"].toString().trimmed();
            int quantity = obj["quantity"].toString().toInt();

            QString tableName = (type == "round") ? "Round_diamond" : "Fancy_diamond";
            QSqlQuery weightQuery(db);
            if (type == "round") {
                bool ok;
                double sizeMMValue = sizeMM.toDouble(&ok);
                if (!ok) continue;
                weightQuery.prepare("SELECT weight FROM Round_diamond WHERE ABS(sizeMM - :sizeMM) = "
                                    "(SELECT MIN(ABS(sizeMM - :sizeMM)) FROM Round_diamond)");
                weightQuery.bindValue(":sizeMM", sizeMMValue);
            } else {
                weightQuery.prepare("SELECT weight FROM Fancy_diamond WHERE LOWER(shape) = LOWER(:type) AND sizeMM = :sizeMM");
                weightQuery.bindValue(":type", type);
                weightQuery.bindValue(":sizeMM", sizeMM);
            }

            if (weightQuery.exec() && weightQuery.next()) {
                double weightPerDiamond = weightQuery.value("weight").toDouble();
                double totalWeightForEntry = quantity * weightPerDiamond;
                weightByType[type] += totalWeightForEntry;
                totalWeight += totalWeightForEntry;
            }
        }

        for (auto it = weightByType.constBegin(); it != weightByType.constEnd(); ++it) {
            detailText += QString("%1\t\t%2ct\n").arg(it.key(), -10).arg(it.value(), 0, 'f', 2);
        }
    }

    db.close();
    return {json, detailText};
}

QPair<QString, QString> DatabaseUtils::fetchStoneDetails(int imageId)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "stone_details_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return {};

    QSqlQuery query(db);
    query.prepare("SELECT stone FROM image_data WHERE image_id = :imageId");
    query.bindValue(":imageId", imageId);

    QString json, detailText;
    double totalWeight = 0.0;
    QMap<QString, double> weightByType;

    if (query.exec() && query.next()) {
        json = query.value("stone").toString();
        QJsonArray array = parseJsonArray(json);
        for (const QJsonValue &value : array) {
            QJsonObject obj = value.toObject();
            QString type = obj["type"].toString().trimmed().toLower();
            QString sizeMM = obj["sizeMM"].toString().trimmed();
            int quantity = obj["quantity"].toString().toInt();

            QSqlQuery weightQuery(db);
            weightQuery.prepare("SELECT weight FROM stones WHERE LOWER(shape) = LOWER(:shape) AND sizeMM = :sizeMM");
            weightQuery.bindValue(":shape", type);
            weightQuery.bindValue(":sizeMM", sizeMM);

            if (weightQuery.exec() && weightQuery.next()) {
                double weightPerStone = weightQuery.value("weight").toDouble();
                double totalWeightForEntry = quantity * weightPerStone;
                weightByType[type] += totalWeightForEntry;
                totalWeight += totalWeightForEntry;
            }
        }

        for (auto it = weightByType.constBegin(); it != weightByType.constEnd(); ++it) {
            detailText += QString("%1\t\t%2ct\n").arg(it.key(), -10).arg(it.value(), 0, 'f', 2);
        }
    }

    db.close();
    return {json, detailText};
}

QString DatabaseUtils::fetchJsonData(int imageId, const QString &type)
{
    // Placeholder implementation; update as needed
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fetch_json_conn");
    db.setDatabaseName(QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db"));
    if (!db.open()) {
        qDebug() << "Error: Failed to open database for fetchJsonData:" << db.lastError().text();
        return "{}";
    }
    QSqlQuery query(db);
    query.prepare("SELECT json_data FROM some_table WHERE image_id = :imageId AND type = :type");
    query.bindValue(":imageId", imageId);
    query.bindValue(":type", type);
    if (!query.exec() || !query.next()) {
        qDebug() << "Error: No JSON data found for imageId =" << imageId << ", type =" << type
                 << ":" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("fetch_json_conn");
        return "{}";
    }
    QString json = query.value(0).toString();
    qDebug() << "Fetched JSON:" << json;
    db.close();
    QSqlDatabase::removeDatabase("fetch_json_conn");
    return json.isEmpty() ? "{}" : json;
}

QPixmap DatabaseUtils::fetchImagePixmap(int imageId)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "image_pixmap_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return QPixmap(":/icon/placeholder.png");

    QSqlQuery query(db);
    query.prepare("SELECT image_path FROM image_data WHERE image_id = :imageId");
    query.bindValue(":imageId", imageId);

    QPixmap pixmap;
    if (query.exec() && query.next()) {
        QString imagePath = query.value("image_path").toString();
        if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
            pixmap.load(imagePath);
        }
    }

    db.close();
    return pixmap.isNull() ? QPixmap(":/icon/placeholder.png") : pixmap;
}

double DatabaseUtils::calculateTotalGoldWeight(const QList<SelectionData> &selections)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "gold_weight_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return 0.0;

    double totalGoldWeight = 0.0;
    QSqlQuery query(db);

    for (const SelectionData &selection : selections) {
        query.prepare("SELECT gold_weight FROM image_data WHERE image_id = :imageId");
        query.bindValue(":imageId", selection.imageId);
        if (query.exec() && query.next()) {
            QString goldWeightJson = query.value("gold_weight").toString();
            QJsonDocument doc = QJsonDocument::fromJson(goldWeightJson.toUtf8());
            if (doc.isArray()) {
                QJsonArray goldWeightArray = doc.array();
                double goldWeight = 0.0;
                for (const QJsonValue &value : goldWeightArray) {
                    QJsonObject obj = value.toObject();
                    if (obj["karat"].toString() == selection.goldType) {
                        goldWeight = obj["weight(g)"].toString().toDouble();
                        break;
                    }
                }
                totalGoldWeight += goldWeight * selection.itemCount;
            }
        }
    }

    db.close();
    return totalGoldWeight;
}

void DatabaseUtils::updateSummaryTable(QTableWidget *table, const QList<SelectionData> &selections, const QString &type)
{
    QStringList headers = {"Shape/Type", "SizeMM", "Quantity", "Weight", "Total Weight"};
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setVisible(true);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: black; color: white; font-weight: bold; font-size: 14px; }");
    table->setRowCount(0);

    QMap<QPair<QString, QString>, QPair<int, double>> aggregates;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "summary_table_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return;

    for (const SelectionData &selection : selections) {
        QSqlQuery query(db);
        query.prepare(QString("SELECT %1 FROM image_data WHERE image_id = :imageId").arg(type));
        query.bindValue(":imageId", selection.imageId);
        if (query.exec() && query.next()) {
            QString json = query.value(0).toString();
            QJsonArray array = parseJsonArray(json);
            for (const QJsonValue &value : array) {
                QJsonObject obj = value.toObject();
                int baseQuantity = obj["quantity"].toString().toInt();
                int adjustedQuantity = baseQuantity * selection.itemCount;
                QString sizeMMStr = obj["sizeMM"].toString();
                QString shape = obj["type"].toString();

                double singleWeight = 0.0;
                QSqlQuery weightQuery(db);
                if (type == "diamond") {
                    QString tableName = (shape == "Round") ? "Round_diamond" : "Fancy_diamond";
                    if (shape == "Round") {
                        double sizeMM = sizeMMStr.toDouble();
                        weightQuery.prepare("SELECT weight FROM Round_diamond WHERE sizeMM = :sizeMM");
                        weightQuery.bindValue(":sizeMM", sizeMM);
                    } else {
                        weightQuery.prepare("SELECT weight FROM Fancy_diamond WHERE shape = :shape AND sizeMM = :sizeMM");
                        weightQuery.bindValue(":shape", shape);
                        weightQuery.bindValue(":sizeMM", sizeMMStr);
                    }
                } else {
                    weightQuery.prepare("SELECT weight FROM stones WHERE shape = :shape AND sizeMM = :sizeMM");
                    weightQuery.bindValue(":shape", shape);
                    weightQuery.bindValue(":sizeMM", sizeMMStr);
                }

                if (weightQuery.exec() && weightQuery.next()) {
                    singleWeight = weightQuery.value("weight").toDouble();
                }

                QPair<QString, QString> key(shape, sizeMMStr);
                QPair<int, double> &aggregate = aggregates[key];
                aggregate.first += adjustedQuantity;
                aggregate.second += singleWeight * adjustedQuantity;
            }
        }
    }

    int totalQuantity = 0;
    double totalWeight = 0.0;

    for (auto it = aggregates.begin(); it != aggregates.end(); ++it) {
        QString shape = it.key().first;
        QString sizeMMStr = it.key().second;
        int rowQuantity = it.value().first;
        double rowTotalWeight = it.value().second;
        double singleWeight = rowQuantity > 0 ? rowTotalWeight / rowQuantity : 0.0;

        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(shape));
        table->setItem(row, 1, new QTableWidgetItem(sizeMMStr));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(rowQuantity)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(singleWeight, 'f', 3) + "ct"));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(rowTotalWeight, 'f', 3) + "ct"));

        totalQuantity += rowQuantity;
        totalWeight += rowTotalWeight;
    }

    if (table->rowCount() > 0) {
        int totalRow = table->rowCount();
        table->insertRow(totalRow);
        table->setItem(totalRow, 0, new QTableWidgetItem("Total"));
        table->setItem(totalRow, 2, new QTableWidgetItem(QString::number(totalQuantity)));
        table->setItem(totalRow, 4, new QTableWidgetItem(QString::number(totalWeight, 'f', 3) + "ct"));
    }

    table->resizeColumnsToContents();
    table->resizeRowsToContents();
    db.close();
    QSqlDatabase::removeDatabase("summary_table_conn");
}

QList<SelectionData> DatabaseUtils::loadUserCart(const QString &userId)
{
    QList<SelectionData> selections;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "load_cart_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) return selections;

    QSqlQuery query(db);
    query.prepare("SELECT cart_details FROM user_cart WHERE user_id = :userId");
    query.bindValue(":userId", userId);

    if (query.exec() && query.next()) {
        QString cartJson = query.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(cartJson.toUtf8());
        if (doc.isArray()) {
            QJsonArray cartArray = doc.array();
            for (const QJsonValue &value : cartArray) {
                QJsonObject obj = value.toObject();
                SelectionData selection;
                selection.imageId = obj["imageId"].toInt();
                selection.goldType = obj["goldType"].toString();
                selection.itemCount = obj["itemCount"].toInt();
                selection.diamondJson = obj["diamondJson"].toString();
                selection.stoneJson = obj["stoneJson"].toString();
                selections.append(selection);
            }
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("load_cart_conn");
    return selections;
}

bool DatabaseUtils::saveUserCart(const QString &userId, const QList<SelectionData> &selections)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "save_cart_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    db.setDatabaseName(dbPath);

    QFile dbFile(dbPath);
    if (!dbFile.exists()) {
        qDebug() << "Error: Database file does not exist at path:" << dbPath;
        return false;
    }

    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery checkTableQuery(db);
    checkTableQuery.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='user_cart'");
    if (!checkTableQuery.exec()) {
        qDebug() << "Error: Failed to check table existence:" << checkTableQuery.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }
    if (!checkTableQuery.next()) {
        qDebug() << "Error: user_cart table does not exist in database.";
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }

    QSqlQuery userCheckQuery(db);
    userCheckQuery.prepare("SELECT user_id FROM users WHERE user_id = :userId");
    userCheckQuery.bindValue(":userId", userId);
    if (!userCheckQuery.exec()) {
        qDebug() << "Error: Failed to check user_id existence:" << userCheckQuery.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }
    if (!userCheckQuery.next()) {
        qDebug() << "Error: user_id" << userId << "does not exist in users table.";
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }

    if (selections.isEmpty()) {
        qDebug() << "Error: No selections provided for user:" << userId;
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }

    db.transaction();

    // Fetch existing pdf_path JSON
    QSqlQuery fetchQuery(db);
    fetchQuery.prepare("SELECT pdf_path FROM user_cart WHERE user_id = :userId");
    fetchQuery.bindValue(":userId", userId);
    QString pdfPathsJson = "[]";
    if (fetchQuery.exec() && fetchQuery.next()) {
        pdfPathsJson = fetchQuery.value("pdf_path").toString();
        if (pdfPathsJson.isEmpty()) pdfPathsJson = "[]";
    }

    // Parse existing JSON
    QJsonDocument doc = QJsonDocument::fromJson(pdfPathsJson.toUtf8());
    QJsonArray pdfArray = doc.isArray() ? doc.array() : QJsonArray();
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // Track existing paths to avoid duplicates
    QSet<QString> existingPaths;
    for (const QJsonValue &value : pdfArray) {
        if (value.isObject()) {
            QString path = value.toObject()["path"].toString();
            if (!path.isEmpty()) {
                // Normalize path
                path = QDir::cleanPath(path);
                existingPaths.insert(path);
            }
        }
    }

    // Append new PDF paths from selections, avoiding duplicates
    for (const auto &selection : selections) {
        if (!selection.pdf_path.isEmpty()) {
            // Normalize and clean the path
            QString normalizedPath = QDir::cleanPath(selection.pdf_path);
            // Verify file exists
            QFile pdfFile(normalizedPath);
            if (pdfFile.exists() && !existingPaths.contains(normalizedPath)) {
                QJsonObject pdfEntry;
                pdfEntry["path"] = QDir::toNativeSeparators(normalizedPath); // Use native separators
                pdfEntry["time"] = currentTime;
                pdfArray.append(pdfEntry);
                existingPaths.insert(normalizedPath);
            } else if (!pdfFile.exists()) {
                qDebug() << "Warning: PDF file does not exist:" << normalizedPath;
            }
        }
    }

    // Convert selections to cart_details JSON
    QJsonArray cartArray;
    for (const SelectionData &selection : selections) {
        QJsonObject obj;
        obj["imageId"] = selection.imageId;
        obj["goldType"] = selection.goldType;
        obj["itemCount"] = selection.itemCount;
        QJsonDocument diamondDoc = QJsonDocument::fromJson(selection.diamondJson.toUtf8());
        obj["diamondJson"] = diamondDoc.isObject() ? diamondDoc.object() : QJsonObject();
        QJsonDocument stoneDoc = QJsonDocument::fromJson(selection.stoneJson.toUtf8());
        obj["stoneJson"] = stoneDoc.isObject() ? stoneDoc.object() : QJsonObject();
        cartArray.append(obj);
    }
    QJsonDocument cartDoc(cartArray);
    QString cartJson = QString(cartDoc.toJson(QJsonDocument::Compact));

    // Delete existing cart entries
    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM user_cart WHERE user_id = :userId");
    deleteQuery.bindValue(":userId", userId);
    if (!deleteQuery.exec()) {
        qDebug() << "Error: Failed to delete from user_cart:" << deleteQuery.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }

    // Insert new cart entry
    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO user_cart (user_id, image_id, cart_details, pdf_path, time) "
                        "VALUES (:userId, :imageId, :cartDetails, :pdfPath, :time)");
    insertQuery.bindValue(":userId", userId);
    insertQuery.bindValue(":imageId", selections.first().imageId);
    insertQuery.bindValue(":cartDetails", cartJson);
    insertQuery.bindValue(":pdfPath", QString(QJsonDocument(pdfArray).toJson(QJsonDocument::Compact)));
    insertQuery.bindValue(":time", currentTime);

    if (!insertQuery.exec()) {
        qDebug() << "Error: Failed to insert into user_cart:" << insertQuery.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }

    if (!db.commit()) {
        qDebug() << "Error: Failed to commit transaction:" << db.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("save_cart_conn");
        return false;
    }

    qDebug() << "Successfully saved cart for user:" << userId << "with pdf_path JSON:" << QString(QJsonDocument(pdfArray).toJson(QJsonDocument::Compact));
    db.close();
    QSqlDatabase::removeDatabase("save_cart_conn");
    return true;
}

QList<QVariantList> DatabaseUtils::fetchUserDetailsForAdmin()
{
    QList<QVariantList> userDetails;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "user_details_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    db.setDatabaseName(dbPath);

    QFile dbFile(dbPath);
    if (!dbFile.exists()) {
        qDebug() << "Error: Database file does not exist at path:" << dbPath;
        return userDetails;
    }

    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return userDetails;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT u.user_id, u.company_name, uc.pdf_path "
        "FROM users u "
        "LEFT JOIN user_cart uc ON u.user_id = uc.user_id"
        );

    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("user_details_conn");
        return userDetails;
    }

    QMap<QString, QVariantList> userMap;
    while (query.next()) {
        QString userId = query.value("user_id").toString();
        QString companyName = query.value("company_name").toString();
        QString pdfPathsJson = query.value("pdf_path").toString();
        QString lastPdfPath;

        if (!pdfPathsJson.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(pdfPathsJson.toUtf8());
            if (doc.isArray()) {
                QJsonArray pdfArray = doc.array();
                if (!pdfArray.isEmpty()) {
                    QJsonObject latestPdf = pdfArray[0].toObject();
                    for (const QJsonValue &value : pdfArray) {
                        QJsonObject pdfObj = value.toObject();
                        if (pdfObj["time"].toString() > latestPdf["time"].toString()) {
                            latestPdf = pdfObj;
                        }
                    }
                    lastPdfPath = latestPdf["path"].toString();
                }
            }
        }

        if (!userMap.contains(userId)) {
            userMap[userId] = {userId, companyName, lastPdfPath};
        } else if (!lastPdfPath.isEmpty() && userMap[userId][2].toString().isEmpty()) {
            userMap[userId][2] = lastPdfPath;
        }
    }

    userDetails = userMap.values();
    db.close();
    QSqlDatabase::removeDatabase("user_details_conn");
    return userDetails;
}

bool DatabaseUtils::deleteUser(const QString &userId)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "delete_user_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    db.setDatabaseName(dbPath);

    // Verify database file exists
    QFile dbFile(dbPath);
    if (!dbFile.exists()) {
        qDebug() << "Error: Database file does not exist at path:" << dbPath;
        return false;
    }

    // Open database connection
    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return false;
    }

    // Begin transaction
    db.transaction();

    // Delete from user_cart
    QSqlQuery query(db);
    query.prepare("DELETE FROM user_cart WHERE user_id = :userId");
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qDebug() << "Error: Failed to delete from user_cart:" << query.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("delete_user_conn");
        return false;
    }

    // Delete from users
    query.prepare("DELETE FROM users WHERE user_id = :userId");
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qDebug() << "Error: Failed to delete from users:" << query.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("delete_user_conn");
        return false;
    }

    // Commit transaction
    if (!db.commit()) {
        qDebug() << "Error: Failed to commit transaction:" << db.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("delete_user_conn");
        return false;
    }

    // Clean up
    db.close();
    QSqlDatabase::removeDatabase("delete_user_conn");
    return true;
}

QList<PdfRecord> DatabaseUtils::getUserPdfs(const QString &userId)
{
    QList<PdfRecord> pdfRecords;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "get_pdfs_conn");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");
    db.setDatabaseName(dbPath);

    QFile dbFile(dbPath);
    if (!dbFile.exists()) {
        qDebug() << "Error: Database file does not exist at path:" << dbPath;
        return pdfRecords;
    }

    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return pdfRecords;
    }

    QSqlQuery query(db);
    query.prepare("SELECT pdf_path FROM user_cart WHERE user_id = :userId AND pdf_path IS NOT NULL");
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qDebug() << "Error: Failed to query pdf_path for user:" << userId << ":" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("get_pdfs_conn");
        return pdfRecords;
    }

    while (query.next()) {
        QString pdfPathsJson = query.value("pdf_path").toString();
        if (pdfPathsJson.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(pdfPathsJson.toUtf8());
        if (!doc.isArray()) {
            qDebug() << "Error: Invalid JSON array for user:" << userId << ", json:" << pdfPathsJson;
            continue;
        }

        QJsonArray pdfArray = doc.array();
        for (const QJsonValue &value : pdfArray) {
            if (!value.isObject()) continue;
            QJsonObject pdfObj = value.toObject();
            PdfRecord record;
            record.pdf_path = pdfObj["path"].toString();
            record.time = pdfObj["time"].toString();
            if (!record.pdf_path.isEmpty()) {
                pdfRecords.append(record);
                qDebug() << "Found PDF for user:" << userId << ", path:" << record.pdf_path << ", time:" << record.time;
            }
        }
    }

    if (pdfRecords.isEmpty()) {
        qDebug() << "No PDFs found for user:" << userId;
    } else {
        // Sort by time descending
        std::sort(pdfRecords.begin(), pdfRecords.end(), [](const PdfRecord &a, const PdfRecord &b) {
            return a.time > b.time;
        });
    }

    db.close();
    QSqlDatabase::removeDatabase("get_pdfs_conn");
    return pdfRecords;
}

QList<QVariantList> DatabaseUtils::fetchJewelryMenuItems()
{
    QList<QVariantList> menuItems;
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "menu_items_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return menuItems;
    }

    QSqlQuery query(db);
    query.prepare("SELECT id, parent_id, name, display_text FROM jewelry_menu ORDER BY parent_id ASC, name ASC");
    if (!query.exec()) {
        qDebug() << "Error: Failed to fetch jewelry menu items:" << query.lastError().text();
        db.close();
        return menuItems;
    }

    while (query.next()) {
        QVariantList item;
        item << query.value("id").toInt();
        item << (query.value("parent_id").isNull() ? -1 : query.value("parent_id").toInt());
        item << query.value("name").toString();
        item << query.value("display_text").toString();
        menuItems.append(item);
    }

    db.close();
    return menuItems;
}

bool DatabaseUtils::insertJewelryMenuItem(int parentId, const QString &name, const QString &displayText)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insert_menu_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO jewelry_menu (parent_id, name, display_text) VALUES (:parent_id, :name, :display_text)");
    if (parentId == -1) {
        query.bindValue(":parent_id", QVariant(QVariant::Int)); // NULL for top-level categories
    } else {
        query.bindValue(":parent_id", parentId);
    }
    query.bindValue(":name", name);
    query.bindValue(":display_text", displayText);

    bool success = query.exec();
    if (!success) {
        qDebug() << "Error: Failed to insert jewelry menu item:" << query.lastError().text();
    }

    db.close();
    return success;
}

bool DatabaseUtils::deleteJewelryMenuItem(int id)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "delete_menu_conn");
    db.setDatabaseName("database/mega_mine_image.db");
    if (!db.open()) {
        qDebug() << "Error: Failed to open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM jewelry_menu WHERE id = :id");
    query.bindValue(":id", id);

    bool success = query.exec();
    if (!success) {
        qDebug() << "Error: Failed to delete jewelry menu item:" << query.lastError().text();
    }

    db.close();
    return success;
}
