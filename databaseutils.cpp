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
#include <QRandomGenerator>

#include "databaseutils.h"
#include "commontypes.h"

//Admin Logic
bool DatabaseUtils::deleteJewelryMenuItem(int id)
{
    const QString connName = "delete_menu_conn";
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) {
            qDebug() << "Error: Failed to open database:" << db.lastError().text();
            return false;
        }

        QSqlQuery query(db);
        query.prepare("DELETE FROM jewelry_menu WHERE id = :id");
        query.bindValue(":id", id);

        success = query.exec();
        if (!success) {
            qDebug() << "Error: Failed to delete jewelry menu item:" << query.lastError().text();
        }

        db.close(); // ✅ Ensure DB is closed before removing
    }

    QSqlDatabase::removeDatabase(connName); // ✅ Proper cleanup
    return success;
}

bool DatabaseUtils::insertJewelryMenuItem(int parentId, const QString &name, const QString &displayText)
{
    const QString connName = "insert_menu_conn";
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/mega_mine_image.db");

        if (!db.open()) {
            qDebug() << "❌ Failed to open DB in insertJewelryMenuItem:" << db.lastError().text();
            return false;
        }

        {
            QSqlQuery query(db);
            query.prepare(R"(
                INSERT INTO jewelry_menu (parent_id, name, display_text)
                VALUES (:parent_id, :name, :display_text)
            )");

            query.bindValue(":parent_id", parentId == -1 ? QVariant(QVariant::Int) : QVariant(parentId));
            query.bindValue(":name", name);
            query.bindValue(":display_text", displayText);

            success = query.exec();
            if (!success) {
                qDebug() << "❌ Insert failed in jewelry_menu:" << query.lastError().text();
            }
        } // ✅ query destroyed here

        db.close();
    }

    QSqlDatabase::removeDatabase(connName); // ✅ Safe cleanup
    return success;
}

QList<QVariantList> DatabaseUtils::fetchJewelryMenuItems()
{
    QList<QVariantList> menuItems;
    const QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");

    const QString connName = "menu_items_conn";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            qWarning() << "❌ Failed to open database:" << db.lastError().text();
            return menuItems;
        }

        QSqlQuery query(db);
        if (!query.exec("SELECT id, parent_id, name, display_text "
                        "FROM jewelry_menu ORDER BY parent_id ASC, name ASC")) {
            qWarning() << "❌ Query failed:" << query.lastError().text();
            db.close();
            return menuItems;
        }

        while (query.next()) {
            QVariantList item;
            item << query.value(0).toInt();
            item << (query.value(1).isNull() ? -1 : query.value(1).toInt());
            item << query.value(2).toString();
            item << query.value(3).toString();
            menuItems.append(item);
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connName);
    return menuItems;
}

QMap<QString, QString> DatabaseUtils::fetchGoldPrices()
{
    const QString connectionName = "gold_conn";
    QMap<QString, QString> prices;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return {};

        {
            QSqlQuery query("SELECT Kt, Price FROM Gold_Price", db);
            if (!query.exec()) return {};

            while (query.next()) {
                prices[query.value(0).toString().trimmed()] = query.value(1).toString();
            }
        } // QSqlQuery destroyed

        // db.close(); // optional, but good
    } // QSqlDatabase destroyed

    QSqlDatabase::removeDatabase(connectionName); // safe now
    return prices;
}

bool DatabaseUtils::updateGoldPrices(const QMap<QString, QString> &priceUpdates)
{
    const QString connectionName = "update_gold_conn";
    bool success = true;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return false;

        db.transaction(); // ✅ start transaction

        QSqlQuery query(db);
        query.prepare("UPDATE Gold_Price SET Price = :price WHERE Kt = :kt");

        for (auto it = priceUpdates.constBegin(); it != priceUpdates.constEnd(); ++it) {
            if (!it.value().isEmpty()) {
                query.bindValue(":price", it.value());
                query.bindValue(":kt", it.key());
                if (!query.exec()) {
                    qWarning() << "❌ Failed to update Gold_Price:" << query.lastError();
                    success = false;
                    break;
                }
            }
        }

        if (success) db.commit();
        else db.rollback();

        db.close(); // optional, explicit
    }

    QSqlDatabase::removeDatabase(connectionName);
    return success;
}

bool DatabaseUtils::sizeMMExists(const QString &table, double sizeMM)
{
    const QString connectionName = "check_conn";
    bool exists = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) {
            qWarning() << "DB open failed (sizeMMExists):" << db.lastError();
            QSqlDatabase::removeDatabase(connectionName);  // ✅ cleanup
            return false;
        }

        QSqlQuery query(db);
        query.prepare("SELECT COUNT(*) FROM " + table + " WHERE sizeMM = :sizeMM");
        query.bindValue(":sizeMM", sizeMM);

        if (query.exec() && query.next()) {
            exists = query.value(0).toInt() > 0;
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return exists;
}

bool DatabaseUtils::insertRoundDiamond(const QString &sieve, double sizeMM, double weight, double price)
{
    const QString connectionName = "insert_round_conn";
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) {
            qWarning() << "DB open failed (insertRoundDiamond):" << db.lastError();
            QSqlDatabase::removeDatabase(connectionName);  // ✅ cleanup
            return false;
        }

        QSqlQuery query(db);
        query.prepare(R"(
            INSERT INTO Round_diamond (sieve, sizeMM, weight, price)
            VALUES (:sieve, :sizeMM, :weight, :price)
        )");
        query.bindValue(":sieve", sieve);
        query.bindValue(":sizeMM", sizeMM);
        query.bindValue(":weight", weight);
        query.bindValue(":price", price);

        success = query.exec();

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return success;
}

bool DatabaseUtils::insertFancyDiamond(const QString &shape, const QString &sizeMM, double weight, double price)
{
    const QString connectionName = "insert_fancy_conn";
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");

        if (!db.open()) {
            qWarning() << "DB open failed (insertFancyDiamond):" << db.lastError();
            QSqlDatabase::removeDatabase(connectionName);  // ✅ cleanup even on failure
            return false;
        }

        QSqlQuery query(db);
        query.prepare(R"(
            INSERT INTO Fancy_diamond (shape, sizeMM, weight, price)
            VALUES (:shape, :sizeMM, :weight, :price)
        )");
        query.bindValue(":shape", shape);
        query.bindValue(":sizeMM", sizeMM);
        query.bindValue(":weight", weight);
        query.bindValue(":price", price);

        if (!query.exec()) {
            qWarning() << "Insert FancyDiamond failed:" << query.lastError();
        } else {
            success = true;
        }

        db.close(); // optional but good hygiene
    } // db out of scope

    QSqlDatabase::removeDatabase(connectionName); // ✅ guaranteed cleanup
    return success;
}

QSqlTableModel* DatabaseUtils::createTableModel(QObject *parent, const QString &table)
{
    QSqlDatabase db;

    if (QSqlDatabase::contains("table_model_conn")) {
        db = QSqlDatabase::database("table_model_conn");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "table_model_conn");
        db.setDatabaseName("database/mega_mine_image.db");
    }

    if (!db.open()) {
        qWarning() << "Failed to open DB for table model:" << db.lastError();
        return nullptr;
    }

    auto *model = new QSqlTableModel(parent, db);
    model->setTable(table);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    if (!model->select()) {
        qWarning() << "Table select failed:" << model->lastError();
        delete model;
        return nullptr;
    }

    return model; // keep DB alive while model exists
}

QStringList DatabaseUtils::fetchRoles()
{
    QStringList roles;
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    const QString connName = "fetch_roles_conn";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/luxeMineAuthentication.db");

        if (!db.open()) {
            qDebug() << "Failed to open database for fetchRoles:" << db.lastError().text();
            return roles;
        }

        {
            QSqlQuery query(db);
            if (!query.exec("SELECT role FROM OrderBook_Roles")) {
                qDebug() << "Failed to execute role query:" << query.lastError().text();
            } else {
                roles << "-";
                while (query.next()) {
                    roles << query.value(0).toString();
                }
            }
        } // QSqlQuery destroyed here

        db.close(); // optional, but keeps things explicit
    } // db handle destroyed here

    QSqlDatabase::removeDatabase(connName); // connection freed from pool
    return roles;
}

QStringList DatabaseUtils::fetchImagePaths()
{
    const QString connectionName = "images_conn";
    QStringList paths;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");

        if (!db.open()) {
            qWarning() << "Failed to open image DB:" << db.lastError();
            return {};
        }

        QSqlQuery query(db);
        if (!query.exec("SELECT image_path FROM image_data")) {
            qWarning() << "Image query failed:" << query.lastError();
            return {};
        }

        paths.reserve(100); // just a guess, avoids multiple reallocations
        while (query.next()) {
            paths.append(query.value(0).toString());
        }
    } // QSqlQuery and QSqlDatabase go out of scope

    QSqlDatabase::removeDatabase(connectionName);
    return paths;
}

bool DatabaseUtils::deleteUser(const QString &userId)
{
    const QString connName = "delete_user_conn";
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");

    if (!QFile::exists(dbPath)) {
        qWarning() << "Database file not found:" << dbPath;
        return false;
    }

    bool success = true;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            qWarning() << "Failed to open DB:" << db.lastError().text();
            return false;
        }

        if (!db.transaction()) {
            qWarning() << "Failed to begin transaction:" << db.lastError().text();
            return false;
        }

        QSqlQuery query(db);

        // delete from user_cart
        query.prepare("DELETE FROM user_cart WHERE user_id = :userId");
        query.bindValue(":userId", userId);
        if (!query.exec()) {
            qWarning() << "Failed to delete from user_cart:" << query.lastError().text();
            db.rollback();
            success = false;
        }

        // delete from users
        if (success) {
            query.prepare("DELETE FROM users WHERE user_id = :userId");
            query.bindValue(":userId", userId);
            if (!query.exec()) {
                qWarning() << "Failed to delete from users:" << query.lastError().text();
                db.rollback();
                success = false;
            }
        }

        // final commit
        if (success && !db.commit()) {
            qWarning() << "Commit failed:" << db.lastError().text();
            db.rollback();
            success = false;
        }
    } // db goes out of scope

    QSqlDatabase::removeDatabase(connName); // ✅ safe now
    return success;
}

QList<QVariantList> DatabaseUtils::fetchUserDetailsForAdmin()
{
    QList<QVariantList> userDetails;
    const QString connName = "user_details_conn";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
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

        QMap<QString, QVariantList> userMap;

        {
            QSqlQuery query(db);
            query.prepare(
                "SELECT u.user_id, u.company_name, uc.pdf_path "
                "FROM users u "
                "LEFT JOIN user_cart uc ON u.user_id = uc.user_id"
                );

            if (!query.exec()) {
                qDebug() << "Error executing query:" << query.lastError().text();
                return userDetails;
            }

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
        }

        userDetails = userMap.values();

    }

    QSqlDatabase::removeDatabase(connName); // ✅ Now safe
    return userDetails;
}

QList<PdfRecord> DatabaseUtils::getUserPdfs(const QString &userId)
{
    QList<PdfRecord> pdfRecords;
    const QString connName = "get_pdfs_conn";
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db");

    if (!QFile::exists(dbPath)) {
        qDebug() << "Error: Database file does not exist at path:" << dbPath;
        return pdfRecords;
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);

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


        db.close();  // Explicitly close before connection removal

    }

    QSqlDatabase::removeDatabase(connName); // ✅ safe now

    if (pdfRecords.isEmpty()) {
        qDebug() << "No PDFs found for user:" << userId;
    } else {
        std::sort(pdfRecords.begin(), pdfRecords.end(), [](const PdfRecord &a, const PdfRecord &b) {
            return a.time > b.time;
        });
    }

    return pdfRecords;
}

bool DatabaseUtils::checkAdminCredentials(const QString &username, const QString &password, QString &role)
{
    const QString connName = "admin_login_conn";
    bool valid = false;

    {
        QDir::setCurrent(QCoreApplication::applicationDirPath());
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/mega_mine.db");

        if (!db.open()) {
            qDebug() << "Failed to connect to admin_login DB:" << db.lastError().text();
            // ✅ ensure cleanup even on failure
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        {
            QSqlQuery query(db);
            query.prepare(R"(
                SELECT role
                FROM admin_login
                WHERE username = :username
                  AND password_hash = :password
            )");
            query.bindValue(":username", username);
            query.bindValue(":password", password);

            if (query.exec() && query.next()) {
                role = query.value("role").toString();
                valid = true;
            } else if (query.lastError().isValid()) {
                qDebug() << "Query error in checkAdminCredentials:"
                         << query.lastError().text();
            }
        }

        db.close(); // optional but fine
    }

    QSqlDatabase::removeDatabase(connName);
    return valid;
}

bool DatabaseUtils::createOrderBookUser(const QString &userId, const QString &userName, const QString &password, const QString &role, const QString &date, QString &errorMsg)
{
    const QString connName = "create_user_conn";
    bool success = false;

    {
        QDir::setCurrent(QCoreApplication::applicationDirPath());
        QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/luxeMineAuthentication.db");

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            errorMsg = "Failed to connect to database: " + db.lastError().text();
            return false;
        }

        // --- Check duplicate userId
        {
            QSqlQuery checkQuery(db);
            checkQuery.prepare("SELECT userId FROM OrderBook_Login WHERE userId = :userId");
            checkQuery.bindValue(":userId", userId);

            if (!checkQuery.exec()) {
                errorMsg = "Failed to check existing user: " + checkQuery.lastError().text();
                db.close();
                QSqlDatabase::removeDatabase(connName);
                return false;
            }

            if (checkQuery.next()) {
                errorMsg = "User ID already exists.";
                db.close();
                QSqlDatabase::removeDatabase(connName);
                return false;
            }
        }

        // --- Insert new user
        {
            QSqlQuery query(db);
            query.prepare(R"(
                INSERT INTO OrderBook_Login (userId, userName, password, date, role)
                VALUES (:userId, :userName, :password, :date, :role)
            )");
            query.bindValue(":userId", userId);
            query.bindValue(":userName", userName);
            query.bindValue(":password", password); // 🔒 Consider hashing this
            query.bindValue(":date", date);
            query.bindValue(":role", role);

            if (!query.exec()) {
                errorMsg = "Failed to save user: " + query.lastError().text();
            } else {
                success = true;
            }
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connName);
    return success;
}

bool DatabaseUtils::updateStatusChangeRequest(int requestId, bool approved, const QString &note)
{
    const QString connName = "status_change_update_conn";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/mega_mine_orderbook.db");

        if (!db.open()) {
            qDebug() << "❌ DB Open Failed:" << db.lastError().text();
            return false; // db handle destroyed automatically
        }

        if (approved) {
            QString jobNo, role, toStatus;

            {
                QSqlQuery selectQuery(db);
                selectQuery.prepare(R"(
                    SELECT jobNo, role, toStatus
                    FROM StatusChangeRequests
                    WHERE id = :id
                )");
                selectQuery.bindValue(":id", requestId);

                if (!selectQuery.exec() || !selectQuery.next()) {
                    qDebug() << "❌ SELECT failed:" << selectQuery.lastError().text();
                    return false;
                }

                jobNo = selectQuery.value("jobNo").toString();
                role = selectQuery.value("role").toString();
                toStatus = selectQuery.value("toStatus").toString();
            } // selectQuery destroyed here

            QStringList roleOrder = {"Manager", "Designer", "Manufacturer", "Accountant"};
            int roleIndex = roleOrder.indexOf(role);
            if (roleIndex == -1) {
                qDebug() << "❌ Invalid role:" << role;
                return false;
            }

            QStringList setParts;
            for (int i = 0; i < roleOrder.size(); ++i) {
                if (i == roleIndex)
                    setParts << QString("%1 = '%2'").arg(roleOrder[i], toStatus);
                else if (i > roleIndex)
                    setParts << QString("%1 = 'Pending'").arg(roleOrder[i]);
            }

            QString updateSQL = QString(R"(
                UPDATE "Order-Status"
                SET %1
                WHERE jobNo = '%2'
            )").arg(setParts.join(", "), jobNo);

            {
                QSqlQuery updateQuery(db);
                if (!updateQuery.exec(updateSQL)) {
                    qDebug() << "❌ Failed to update Order-Status:" << updateQuery.lastError().text();
                    return false;
                }
            } // updateQuery destroyed here
        }

        {
            QSqlQuery finalUpdateQuery(db);
            if (approved) {
                finalUpdateQuery.prepare(R"(
                    UPDATE StatusChangeRequests
                    SET status = 'Approved'
                    WHERE id = :id
                )");
                finalUpdateQuery.bindValue(":id", requestId);
            } else {
                finalUpdateQuery.prepare(R"(
                    UPDATE StatusChangeRequests
                    SET status = 'Declined',
                        note = :note
                    WHERE id = :id
                )");
                finalUpdateQuery.bindValue(":note", note);
                finalUpdateQuery.bindValue(":id", requestId);
            }

            if (!finalUpdateQuery.exec()) {
                qDebug() << "❌ Failed to update StatusChangeRequests:" << finalUpdateQuery.lastError().text();
                return false;
            }
        } // finalUpdateQuery destroyed here

        db.close(); // optional, but makes intent explicit
    } // db handle destroyed here

    QSqlDatabase::removeDatabase(connName); // now safe — all queries destroyed
    return true;
}

bool DatabaseUtils::updateRoleStatus(const QString &jobNo, const QString &fieldName, const QString &newStatus)
{
    const QString connName =
        QStringLiteral("update_role_status_conn_%1")
            .arg(QRandomGenerator::global()->generate());

    QDir::setCurrent(QCoreApplication::applicationDirPath());

    {
        // Scoped block to ensure 'db' and 'query' are destroyed before removeDatabase()
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/mega_mine_orderbook.db");

        if (!db.open()) {
            qDebug() << "[updateRoleStatus] DB open failed:" << db.lastError().text();
            return false; // Will remove later
        }

        QString sql = QString(R"(
            UPDATE "Order-Status"
            SET %1 = :status
            WHERE jobNo = :jobNo
        )").arg(fieldName);

        QSqlQuery query(db);
        query.prepare(sql);
        query.bindValue(":status", newStatus);
        query.bindValue(":jobNo", jobNo);

        bool success = query.exec();
        if (!success) {
            qDebug() << "[-] Update failed:" << query.lastError().text();
            qDebug() << "[:(] SQL Tried:" << query.lastQuery();
        } else {
            qDebug() << "[+] Status updated:" << fieldName << "=" << newStatus
                     << "for JobNo:" << jobNo;
        }

        db.close();
        if (!success) {
            // Let scope exit happen before removeDatabase
            QSqlDatabase::removeDatabase(connName);
            return false;
        }
    }

    // Now safe to remove the database connection
    QSqlDatabase::removeDatabase(connName);
    return true;
}

QList<JobSheetRequest> DatabaseUtils::fetchJobSheetRequests()
{
    QList<JobSheetRequest> results;
    const QString connName = "fetch_jobsheet_requests_conn";

    {
        QString dbPath = QCoreApplication::applicationDirPath() + "/database/mega_mine_orderbook.db";
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            qDebug() << "[fetchJobSheetRequests] ❌ DB open failed:" << db.lastError().text();
            // db will go out of scope before removeDatabase()
        } else {
            QString queryStr = R"(
                SELECT
                    D.sellerId,
                    D.partyId,
                    D.jobNo,
                    O.Manager,
                    O.Designer,
                    O.Manufacturer,
                    O.Accountant,
                    S.id AS requestId,
                    S.role,
                    S.userId,
                    S.fromStatus,
                    S.toStatus,
                    S.requestTime
                FROM "OrderBook-Detail" D
                JOIN "Order-Status" O ON D.jobNo = O.jobNo
                LEFT JOIN (
                    SELECT r.*
                    FROM StatusChangeRequests r
                    WHERE r.status = 'Pending'
                      AND r.requestTime = (
                          SELECT MAX(requestTime)
                          FROM StatusChangeRequests
                          WHERE jobNo = r.jobNo
                            AND status = 'Pending'
                      )
                ) S ON D.jobNo = S.jobNo;
            )";

            QSqlQuery query(db);
            if (!query.exec(queryStr)) {
                qDebug() << "[fetchJobSheetRequests] ❌ Query failed:" << query.lastError().text();
            } else {
                while (query.next()) {
                    JobSheetRequest row;
                    row.sellerId        = query.value(0).toString();
                    row.partyId         = query.value(1).toString();
                    row.jobNo           = query.value(2).toString();
                    row.manager         = query.value(3).toString();
                    row.designer        = query.value(4).toString();
                    row.manufacturer    = query.value(5).toString();
                    row.accountant      = query.value(6).toString();
                    row.requestId       = query.value(7).toInt();
                    row.requestRole     = query.value(8).toString();
                    row.requestRoleId   = query.value(9).toString();
                    row.fromStatus      = query.value(10).toString();
                    row.toStatus        = query.value(11).toString();
                    row.requestTime     = query.value(12).toDateTime().toString(Qt::ISODate);
                    results.append(row);
                }
            }
            // query destroyed here

            db.close();
        }
        // db destroyed here
    }

    // Now safe to remove connection
    QSqlDatabase::removeDatabase(connName);

    return results;
}


//User Logic
bool DatabaseUtils::userExists(const QString &userId)
{
    const QString connectionName = "user_check_conn";
    bool exists = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return false;

        {
            QSqlQuery query(db);
            query.prepare("SELECT user_id FROM users WHERE user_id = :user_id");
            query.bindValue(":user_id", userId);
            exists = query.exec() && query.next();
        } // ✅ QSqlQuery destroyed here

        db.close(); // optional
    } // ✅ QSqlDatabase destroyed here

    QSqlDatabase::removeDatabase(connectionName); // ✅ now safe
    return exists;
}

bool DatabaseUtils::userExistsByMobileAndName(const QString &userId, const QString &name)
{
    const QString connectionName = "user_mobile_check_conn";
    bool exists = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return false;

        {
            QSqlQuery query(db);
            query.prepare("SELECT user_id FROM users WHERE mobile_no = :mobile_no AND name = :name");
            query.bindValue(":mobile_no", userId);
            query.bindValue(":name", name);
            exists = query.exec() && query.next();
        } // ✅ QSqlQuery destroyed here

        db.close(); // optional but safe
    } // ✅ QSqlDatabase destroyed here

    QSqlDatabase::removeDatabase(connectionName); // ✅ now safe
    return exists;
}

bool DatabaseUtils::insertUser(const QString &userId, const QString &companyName, const QString &mobileNo, const QString &gstNo, const QString &name, const QString &emailId, const QString &address)
{
    const QString connectionName = "user_insert_conn";
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return false;

        {
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

            success = query.exec();
        } // ✅ QSqlQuery destroyed here

        db.close(); // optional but safe
    } // ✅ QSqlDatabase destroyed here

    QSqlDatabase::removeDatabase(connectionName); // ✅ now safe
    return success;
}

QList<SelectionData> DatabaseUtils::loadUserCart(const QString &userId)
{
    QList<SelectionData> selections;
    const QString connectionName = "load_cart_conn";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return selections;

        {
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
                        selection.imageId    = obj["imageId"].toInt();
                        selection.goldType   = obj["goldType"].toString();
                        selection.itemCount  = obj["itemCount"].toInt();
                        selection.diamondJson = obj["diamondJson"].toString();
                        selection.stoneJson   = obj["stoneJson"].toString();
                        selection.pdf_path    = obj["pdf_path"].toString();   // ✅ restore pdf_path
                        selections.append(selection);
                    }
                }
            }
        } // ✅ QSqlQuery destroyed here

        db.close();
    } // ✅ db destroyed

    QSqlDatabase::removeDatabase(connectionName);
    return selections;
}

bool DatabaseUtils::saveUserCart(const QString &userId, const QList<SelectionData> &selections)
{
    const QString connName = "save_cart_conn";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
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

        // --- Check if table exists
        {
            QSqlQuery checkTableQuery(db);
            checkTableQuery.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='user_cart'");
            if (!checkTableQuery.exec() || !checkTableQuery.next()) {
                qDebug() << "Error: user_cart table does not exist.";
                return false;
            }
        }

        // --- Check user exists
        {
            QSqlQuery userCheckQuery(db);
            userCheckQuery.prepare("SELECT user_id FROM users WHERE user_id = :userId");
            userCheckQuery.bindValue(":userId", userId);
            if (!userCheckQuery.exec() || !userCheckQuery.next()) {
                qDebug() << "Error: user_id" << userId << "does not exist in users table.";
                return false;
            }
        }

        if (selections.isEmpty()) {
            QSqlQuery deleteQuery(db);
            deleteQuery.prepare("DELETE FROM user_cart WHERE user_id = :userId");
            deleteQuery.bindValue(":userId", userId);

            if (!deleteQuery.exec()) {
                qDebug() << "Error: Failed to delete empty cart for user:" << userId
                         << deleteQuery.lastError().text();
                db.close();
                QSqlDatabase::removeDatabase(connName);   // ✅ cleanup even on failure
                return false;
            }

            db.close();
            QSqlDatabase::removeDatabase(connName);   // ✅ cleanup on success
            qDebug() << "Cart cleared for user:" << userId;
            return true;
        }


        db.transaction();

        // --- Fetch existing pdf_path
        QString pdfPathsJson = "[]";
        {
            QSqlQuery fetchQuery(db);
            fetchQuery.prepare("SELECT pdf_path FROM user_cart WHERE user_id = :userId");
            fetchQuery.bindValue(":userId", userId);
            if (fetchQuery.exec() && fetchQuery.next()) {
                pdfPathsJson = fetchQuery.value(0).toString();
                if (pdfPathsJson.isEmpty()) pdfPathsJson = "[]";
            }
        }

        // --- Parse existing JSON
        QJsonArray pdfArray = QJsonDocument::fromJson(pdfPathsJson.toUtf8()).array();
        QSet<QString> existingPaths;
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        for (const QJsonValue &val : pdfArray) {
            QString path = val.toObject()["path"].toString();
            if (!path.isEmpty()) {
                existingPaths.insert(QDir::cleanPath(path));
            }
        }

        for (const SelectionData &selection : selections) {
            if (!selection.pdf_path.isEmpty()) {
                QString normPath = QDir::cleanPath(selection.pdf_path);
                if (QFile::exists(normPath) && !existingPaths.contains(normPath)) {
                    QJsonObject pdfEntry;
                    pdfEntry["path"] = QDir::toNativeSeparators(normPath);
                    pdfEntry["time"] = currentTime;
                    pdfArray.append(pdfEntry);
                    existingPaths.insert(normPath);
                } else if (!QFile::exists(normPath)) {
                    qDebug() << "Warning: PDF file does not exist:" << normPath;
                }
            }
        }

        // --- Prepare cart JSON
        QJsonArray cartArray;
        for (const SelectionData &s : selections) {
            QJsonObject obj;
            obj["imageId"]     = s.imageId;
            obj["goldType"]    = s.goldType;
            obj["itemCount"]   = s.itemCount;
            obj["diamondJson"] = s.diamondJson;  // ✅ store as string
            obj["stoneJson"]   = s.stoneJson;    // ✅ store as string
            obj["pdf_path"]    = s.pdf_path;     // ✅ also keep in cart JSON
            cartArray.append(obj);
        }
        QString cartJson = QString(QJsonDocument(cartArray).toJson(QJsonDocument::Compact));

        QString pdfJson = QString(QJsonDocument(pdfArray).toJson(QJsonDocument::Compact));

        // --- Delete previous
        {
            QSqlQuery deleteQuery(db);
            deleteQuery.prepare("DELETE FROM user_cart WHERE user_id = :userId");
            deleteQuery.bindValue(":userId", userId);
            if (!deleteQuery.exec()) {
                qDebug() << "Error: Failed to delete from user_cart:" << deleteQuery.lastError().text();
                db.rollback();
                return false;
            }
        }

        // --- Insert new
        {
            QSqlQuery insertQuery(db);
            insertQuery.prepare("INSERT INTO user_cart (user_id, image_id, cart_details, pdf_path, time) "
                                "VALUES (:userId, :imageId, :cartDetails, :pdfPath, :time)");
            insertQuery.bindValue(":userId", userId);
            insertQuery.bindValue(":imageId", selections.first().imageId);
            insertQuery.bindValue(":cartDetails", cartJson);
            insertQuery.bindValue(":pdfPath", pdfJson);
            insertQuery.bindValue(":time", currentTime);

            if (!insertQuery.exec()) {
                qDebug() << "Error: Failed to insert into user_cart:" << insertQuery.lastError().text();
                db.rollback();
                return false;
            }
        }

        if (!db.commit()) {
            qDebug() << "Error: Commit failed:" << db.lastError().text();
            db.rollback();
            return false;
        }

        qDebug() << "Successfully saved cart for user:" << userId;
    }

    QSqlDatabase::removeDatabase(connName); // ✅ safe now
    return true;
}

QList<ImageRecord> DatabaseUtils::getAllItems()
{
    QList<ImageRecord> items;
    const QString connName = "get_all_items_conn";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) {
            qDebug() << "Error: Could not open database for getAllItems:" << db.lastError().text();
            return items;
        }

        {
            QSqlQuery query(db);
            query.prepare("SELECT image_id, image_path, image_type, design_no, company_name, gold_weight, diamond, stone, time, note FROM image_data");

            if (query.exec()) {
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
            } else {
                qDebug() << "Error: Failed to execute query in getAllItems:" << query.lastError().text();
            }
        } // ✅ query destroyed here before db.close()

        db.close();
    }
    QSqlDatabase::removeDatabase(connName); // Safe to remove after scope ends
    return items;
}

QPixmap DatabaseUtils::fetchImagePixmap(int imageId)
{
    const QString connectionName = "image_pixmap_conn";
    QPixmap pixmap;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath())
                               .filePath("database/mega_mine_image.db"));
        if (!db.open()) {
            qDebug() << "Failed to open DB for fetchImagePixmap:" << db.lastError().text();
            return QPixmap(":/icon/placeholder.png");
        }

        {
            QSqlQuery query(db);
            query.prepare("SELECT image_path FROM image_data WHERE image_id = :imageId");
            query.bindValue(":imageId", imageId);

            if (query.exec() && query.next()) {
                const QString imagePath = query.value(0).toString();
                if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
                    if (!pixmap.load(imagePath)) {
                        qWarning() << "Failed to load image from:" << imagePath;
                    }
                }
            } else {
                qDebug() << "No image path found for id:" << imageId << query.lastError().text();
            }
        } // ✅ query destroyed

        db.close();
    } // ✅ db destroyed

    QSqlDatabase::removeDatabase(connectionName);
    return pixmap.isNull() ? QPixmap(":/icon/placeholder.png") : pixmap;
}

QString DatabaseUtils::fetchJsonData(int imageId, const QString &column)
{
    if (column != "diamond" && column != "stone") {
        qWarning() << "Invalid column requested in fetchJsonData:" << column;
        return "[]";
    }

    const QString connectionName = "fetch_json_conn";
    QString json = "[]";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_image.db"));
        if (!db.open()) {
            qDebug() << "Error: Failed to open database for fetchJsonData:" << db.lastError().text();
            return "[]";
        }

        {
            QSqlQuery query(db);
            query.prepare(QString("SELECT %1 FROM image_data WHERE image_id = :imageId").arg(column));
            query.bindValue(":imageId", imageId);

            if (query.exec() && query.next()) {
                json = query.value(0).toString();
            } else {
                qDebug() << "No JSON data found for imageId =" << imageId << ", column =" << column;
            }
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return json.isEmpty() ? "[]" : json;
}

QPair<QString, QString> DatabaseUtils::fetchDiamondDetails(int imageId)
{
    const QString connectionName = "diamond_details_conn";
    QString json, detailText;
    double totalWeight = 0.0;
    QMap<QString, double> weightByType;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return {};

        {
            QSqlQuery query(db);
            query.prepare("SELECT diamond FROM image_data WHERE image_id = :imageId");
            query.bindValue(":imageId", imageId);

            if (query.exec() && query.next()) {
                json = query.value("diamond").toString();

                // ✅ validate JSON before using
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
                    json.clear(); // avoid passing invalid JSON
                } else {
                    QJsonArray array = doc.array();

                    for (const QJsonValue &value : array) {
                        if (!value.isObject()) continue;

                        QJsonObject obj = value.toObject();
                        QString type = obj["type"].toString().trimmed().toLower();
                        QString sizeMM = obj["sizeMM"].toString().trimmed();
                        int quantity = obj["quantity"].toString().toInt();
                        {
                            QSqlQuery weightQuery(db);
                            if (type == "round") {
                                bool ok;
                                double sizeMMValue = sizeMM.toDouble(&ok);
                                if (!ok) continue;

                                weightQuery.prepare(R"(
                                    SELECT weight FROM Round_diamond
                                    WHERE ABS(sizeMM - :sizeMM) =
                                        (SELECT MIN(ABS(sizeMM - :sizeMM)) FROM Round_diamond)
                                )");
                                weightQuery.bindValue(":sizeMM", sizeMMValue);
                            } else {
                                weightQuery.prepare("SELECT weight FROM Fancy_diamond WHERE LOWER(shape) = LOWER(:type) AND sizeMM = :sizeMM");
                                weightQuery.bindValue(":type", type);
                                weightQuery.bindValue(":sizeMM", sizeMM);
                            }

                            if (weightQuery.exec() && weightQuery.next()) {
                                double weightPerDiamond = weightQuery.value("weight").toDouble();
                                double totalWeightForEntry = quantity * weightPerDiamond;
                                // qDebug()<<quantity<<weightPerDiamond;///////
                                weightByType[type] += totalWeightForEntry;
                                totalWeight += totalWeightForEntry;
                                // qDebug()<<totalWeight;
                            }
                        } // ✅ weightQuery destroyed here
                    }
                    // qDebug()<<json<<"-----"<<detailText;
                    for (auto it = weightByType.constBegin(); it != weightByType.constEnd(); ++it) {
                        // qDebug()<<it;
                        // qDebug()<<it.key()<<it.value();
                        detailText += QString("%1\t\t%2ct\n").arg(it.key(), -10).arg(it.value(), 0, 'f', 2);
                    }
                }
            }
        } // ✅ outer query destroyed

        db.close();
    } // ✅ QSqlDatabase destroyed

    QSqlDatabase::removeDatabase(connectionName);

    return {json, detailText};
}

QPair<QString, QString> DatabaseUtils::fetchStoneDetails(int imageId)
{
    const QString connectionName = "stone_details_conn";
    QString json, detailText;
    double totalWeight = 0.0;
    QMap<QString, double> weightByType;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return {};

        {
            QSqlQuery query(db);
            query.prepare("SELECT stone FROM image_data WHERE image_id = :imageId");
            query.bindValue(":imageId", imageId);

            if (query.exec() && query.next()) {
                json = query.value("stone").toString();

                // ✅ validate JSON before using
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                    QJsonArray array = doc.array();

                    for (const QJsonValue &value : array) {
                        if (!value.isObject()) continue;

                        QJsonObject obj = value.toObject();
                        QString type = obj["type"].toString().trimmed().toLower();
                        QString sizeMM = obj["sizeMM"].toString().trimmed();
                        int quantity = obj["quantity"].toString().toInt(); // ✅ simpler & safer

                        {
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
                        } // ✅ weightQuery destroyed here
                    }

                    for (auto it = weightByType.constBegin(); it != weightByType.constEnd(); ++it) {
                        detailText += QString("%1\t\t%2ct\n").arg(it.key(), -10).arg(it.value(), 0, 'f', 2);
                    }
                } else {
                    json.clear(); // ✅ don't store bad JSON
                }
            }
        } // ✅ query destroyed

        db.close();
    } // ✅ db destroyed

    QSqlDatabase::removeDatabase(connectionName);
    return {json, detailText};
}

double DatabaseUtils::calculateTotalGoldWeight(const QList<SelectionData> &selections)
{
    const QString connectionName = "gold_weight_conn";
    double totalGoldWeight = 0.0;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return 0.0;

        QHash<int, QString> goldCache;
        QSqlQuery query(db);

        for (const SelectionData &selection : selections) {
            if (!goldCache.contains(selection.imageId)) {
                query.prepare("SELECT gold_weight FROM image_data WHERE image_id = :imageId");
                query.bindValue(":imageId", selection.imageId);
                if (query.exec() && query.next()) {
                    goldCache[selection.imageId] = query.value(0).toString();
                }
            }

            QString goldWeightJson = goldCache.value(selection.imageId);
            QJsonDocument doc = QJsonDocument::fromJson(goldWeightJson.toUtf8());
            if (doc.isArray()) {
                for (const QJsonValue &value : doc.array()) {
                    QJsonObject obj = value.toObject();
                    if (obj["karat"].toString() == selection.goldType) {
                        totalGoldWeight += obj["weight(g)"].toString().toDouble() * selection.itemCount;
                        break;
                    }
                }
            }
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return totalGoldWeight;
}

QJsonObject DatabaseUtils::parseGoldJson(const QString &goldJson)
{
    QJsonObject goldData;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(goldJson.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isArray())
        return goldData;

    QJsonArray goldArray = doc.array();
    for (const QJsonValue &value : goldArray) {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();
        QString karat = obj["karat"].toString();

        double weight = 0.0;
        QJsonValue weightValue = obj["weight(g)"];
        if (weightValue.isDouble()) {
            weight = weightValue.toDouble();
        } else if (weightValue.isString()) {
            weight = weightValue.toString().toDouble();
        }

        if (!karat.isEmpty())
            goldData[karat] = weight;
    }

    return goldData;
}

QJsonArray DatabaseUtils::parseJsonArray(const QString &json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.isArray() ? doc.array() : QJsonArray();
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
    const QString connectionName = "summary_table_conn";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("database/mega_mine_image.db");
        if (!db.open()) return;

        {
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

                        {
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
                        } // ✅ weightQuery destroyed here

                        QPair<QString, QString> key(shape, sizeMMStr);
                        QPair<int, double> &aggregate = aggregates[key];
                        aggregate.first += adjustedQuantity;
                        aggregate.second += singleWeight * adjustedQuantity;
                    }
                }
            }
        } // ✅ all QSqlQuery destroyed here

        db.close(); // optional
    } // ✅ db destroyed here

    QSqlDatabase::removeDatabase(connectionName); // ✅ now safe

    // Fill table
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
}


//AddCatalog Logic
QStringList DatabaseUtils::fetchShapes(const QString &tableType)
{
    const QString connectionName = "shapes_conn";
    QStringList shapes;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(QCoreApplication::applicationDirPath() + "/database/mega_mine_image.db");

    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return {};
    }

    {
        QSqlQuery query(db);
        QString queryStr = (tableType == "diamond")
                               ? "SELECT DISTINCT shape FROM Fancy_diamond UNION SELECT 'Round' FROM Round_diamond"
                               : "SELECT DISTINCT shape FROM stones";

        if (!query.exec(queryStr)) {
            db.close();
            QSqlDatabase::removeDatabase(connectionName);
            return {};
        }

        while (query.next()) {
            shapes.append(query.value(0).toString());
        }
    }

    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return shapes;
}

QStringList DatabaseUtils::fetchSizes(const QString &tableType, const QString &shape)
{
    const QString connectionName = "sizes_conn";
    QStringList sizes;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(QCoreApplication::applicationDirPath() + "/database/mega_mine_image.db");

    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return {};
    }

    {
        QSqlQuery query(db);

        if (tableType == "diamond") {
            if (shape == "Round") {
                query.prepare("SELECT DISTINCT sizeMM FROM Round_diamond ORDER BY sizeMM");
            } else {
                query.prepare("SELECT DISTINCT sizeMM FROM Fancy_diamond WHERE shape = :shape ORDER BY sizeMM");
                query.bindValue(":shape", shape);
            }
        } else {
            query.prepare("SELECT DISTINCT sizeMM FROM stones WHERE shape = :shape ORDER BY sizeMM");
            query.bindValue(":shape", shape);
        }

        if (!query.exec()) {
            db.close();
            QSqlDatabase::removeDatabase(connectionName);
            return {};
        }

        while (query.next()) {
            sizes.append(query.value(0).toString());
        }
    }

    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return sizes;
}

QString DatabaseUtils::saveImage(const QString &imagePath)
{
    if (!QFile::exists(imagePath))
        return {};

    QString targetDir = QDir(QCoreApplication::applicationDirPath()).filePath("images");
    QDir().mkpath(targetDir);

    QString newImagePath = QDir(targetDir).filePath(QFileInfo(imagePath).fileName());

    // Handle existing file (overwrite safely)
    if (QFile::exists(newImagePath)) {
        QFile::remove(newImagePath);
    }

    if (!QFile::copy(imagePath, newImagePath))
        return {};

    return newImagePath;
}


bool DatabaseUtils::insertCatalogData(const QString &imagePath, const QString &imageType, const QString &designNo,
                                      const QString &companyName, const QJsonArray &goldArray,
                                      const QJsonArray &diamondArray, const QJsonArray &stoneArray,
                                      const QString &note)
{
    const QString connectionName = "insert_conn";
    bool success = false;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(QCoreApplication::applicationDirPath() + "/database/mega_mine_image.db");

    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    QJsonDocument goldDoc(goldArray);
    QJsonDocument diamondDoc(diamondArray);
    QJsonDocument stoneDoc(stoneArray);

    {
        QSqlQuery query(db);
        query.prepare(R"(
            INSERT INTO image_data
            (image_path, image_type, design_no, company_name, gold_weight, diamond, stone, time, note)
            VALUES (:image_path, :image_type, :design_no, :company_name, :gold_weight, :diamond, :stone, :time, :note)
        )");

        query.bindValue(":image_path", imagePath);
        query.bindValue(":image_type", imageType);
        query.bindValue(":design_no", designNo);
        query.bindValue(":company_name", companyName);
        query.bindValue(":gold_weight", goldDoc.toJson(QJsonDocument::Compact));
        query.bindValue(":diamond", diamondDoc.toJson(QJsonDocument::Compact));
        query.bindValue(":stone", stoneDoc.toJson(QJsonDocument::Compact));
        query.bindValue(":time", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.bindValue(":note", note);

        if (!query.exec()) {
            qDebug() << "Insert failed:" << query.lastError().text();
        } else {
            success = true;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return success;
}








//not fixed

int DatabaseUtils::insertDummyOrder(const QString &sellerName, const QString &sellerId, const QString &partyName) {
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insert_order_conn");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB:" << db.lastError().text();
        return -1;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO "OrderBook-Detail"
        (sellerName, sellerId, partyId, partyName, jobNo, orderNo, orderDate, deliveryDate)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(sellerName);                                       // sellerName
    query.addBindValue(sellerId);                                         // sellerId
    query.addBindValue("TEMP_ID");                                        // partyId (dummy)
    query.addBindValue(partyName);                                        // partyName
    query.addBindValue("TEMP_JOB");                                       // jobNo
    query.addBindValue("TEMP_ORDER");                                     // orderNo
    query.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));      // orderDate
    query.addBindValue(QDate::currentDate().addDays(1).toString("yyyy-MM-dd")); // deliveryDate

    if (!query.exec()) {
        qDebug() << "❌ Insert failed:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("insert_order_conn");
        return -1;
    }

    int newId = query.lastInsertId().toInt();
    db.close();
    QSqlDatabase::removeDatabase("insert_order_conn");
    return newId;
}

bool DatabaseUtils::updateDummyOrder(int orderId, const QString &jobNo, const QString &orderNo) {
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "update_order_conn");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "Failed to open DB for update:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE "OrderBook-Detail"
        SET jobNo = :jobNo, orderNo = :orderNo
        WHERE id = :orderId
    )");

    query.bindValue(":jobNo", jobNo);
    query.bindValue(":orderNo", orderNo);
    query.bindValue(":orderId", orderId);

    bool success = query.exec();
    if (!success) {
        qDebug() << "❌ Update failed:" << query.lastError().text();
    } else if (query.numRowsAffected() == 0) {
        qDebug() << "⚠️ No row updated: Check orderId:" << orderId;
        success = false;
    }

    db.close();
    QSqlDatabase::removeDatabase("update_order_conn");
    return success;
}

int DatabaseUtils::getNextJobNumber() {
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "next_job_conn");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB in getNextJobNumber:" << db.lastError().text();
        return 1;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT jobNo FROM "OrderBook-Detail"
        WHERE jobNo LIKE 'JOB%' AND LENGTH(jobNo) > 3
        ORDER BY CAST(SUBSTR(jobNo, 4) AS INTEGER) DESC
        LIMIT 1
    )");

    int nextJobNumber = 1;
    if (query.exec() && query.next()) {
        QString lastJobNo = query.value(0).toString(); // e.g., "JOB00023"
        bool ok;
        int number = lastJobNo.mid(3).toInt(&ok);      // "00023" → 23
        if (ok) nextJobNumber = number + 1;
    } else {
        qDebug() << "ℹ️ No previous jobNo found. Starting from 1.";
    }

    db.close();
    QSqlDatabase::removeDatabase("next_job_conn");
    return nextJobNumber;
}


int DatabaseUtils::getNextOrderNumberForSeller(const QString &sellerId) {
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "next_order_conn");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB in getNextOrderNumberForSeller:" << db.lastError().text();
        return 1;
    }

    QSqlQuery query(db);
    // Extract numeric part after sellerId and cast to integer, then order by it
    query.prepare(QString(R"(
        SELECT orderNo FROM "OrderBook-Detail"
        WHERE orderNo LIKE :prefix AND LENGTH(orderNo) > :minLen
        ORDER BY CAST(SUBSTR(orderNo, :startPos) AS INTEGER) DESC
        LIMIT 1
    )"));

    query.bindValue(":prefix", sellerId + "%");
    query.bindValue(":minLen", sellerId.length());
    query.bindValue(":startPos", sellerId.length() + 1); // SQLite is 1-based indexing

    int nextOrder = 1;
    if (query.exec() && query.next()) {
        QString lastOrder = query.value(0).toString(); // e.g., SELL12300007
        QString numericPart = lastOrder.mid(sellerId.length()); // "00007"
        bool ok;
        int number = numericPart.toInt(&ok);
        if (ok) nextOrder = number + 1;
    } else {
        qDebug() << "ℹ️ No previous orderNo found for seller:" << sellerId;
    }

    db.close();
    QSqlDatabase::removeDatabase("next_order_conn");
    return nextOrder;
}


bool DatabaseUtils::saveOrder(const OrderData &order) {
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "save_order_conn");
    db.setDatabaseName("database/mega_mine_orderbook.db");

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB in saveOrder:" << db.lastError().text();
        return false;
    }

    if (!db.transaction()) {
        qDebug() << "❌ Failed to start transaction:" << db.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("save_order_conn");
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE "OrderBook-Detail" SET
            sellerName = :sellerName, sellerId = :sellerId, partyId = :partyId, partyName = :partyName,
            jobNo = :jobNo, orderNo = :orderNo,
            clientId = :clientId, agencyId = :agencyId, shopId = :shopId, reteailleId = :retailleId, starId = :starId,
            address = :address, city = :city, state = :state, country = :country,
            orderDate = :orderDate, deliveryDate = :deliveryDate,
            productName = :productName, productPis = :productPis, approxProductWt = :approxProductWt, metalPrice = :metalPrice,
            metalName = :metalName, metalPurity = :metalPurity, metalColor = :metalColor,
            sizeNo = :sizeNo, sizeMM = :sizeMM, length = :length, width = :width, height = :height,
            diaPacific = :diaPacific, diaPurity = :diaPurity, diaColor = :diaColor, diaPrice = :diaPrice,
            stPacific = :stPacific, stPurity = :stPurity, stColor = :stColor, stPrice = :stPrice,
            designNo1 = :designNo1, designNo2 = :designNo2,
            image1Path = :image1Path, image2Path = :image2Path,
            metalCertiName = :metalCertiName, metalCertiType = :metalCertiType,
            diaCertiName = :diaCertiName, diaCertiType = :diaCertiType,
            pesSaki = :pesSaki, chainLock = :chainLock, polish = :polish,
            settingLebour = :settingLabour, metalStemp = :metalStemp, paymentMethod = :paymentMethod,
            totalAmount = :totalAmount, advance = :advance, remaining = :remaining,
            note = :note, extraDetail = :extraDetail,
            isSaved = 1
        WHERE id = :id
    )");

    // Bind all values
    query.bindValue(":sellerName", order.sellerName);
    query.bindValue(":sellerId", order.sellerId);
    query.bindValue(":partyId", order.partyId);
    query.bindValue(":partyName", order.partyName);
    query.bindValue(":jobNo", order.jobNo);
    query.bindValue(":orderNo", order.orderNo);
    query.bindValue(":clientId", order.clientId);
    query.bindValue(":agencyId", order.agencyId);
    query.bindValue(":shopId", order.shopId);
    query.bindValue(":retailleId", order.retailleId);
    query.bindValue(":starId", order.starId);
    query.bindValue(":address", order.address);
    query.bindValue(":city", order.city);
    query.bindValue(":state", order.state);
    query.bindValue(":country", order.country);
    query.bindValue(":orderDate", order.orderDate);
    query.bindValue(":deliveryDate", order.deliveryDate);
    query.bindValue(":productName", order.productName);
    query.bindValue(":productPis", order.productPis);
    query.bindValue(":approxProductWt", order.approxProductWt);
    query.bindValue(":metalPrice", order.metalPrice);
    query.bindValue(":metalName", order.metalName);
    query.bindValue(":metalPurity", order.metalPurity);
    query.bindValue(":metalColor", order.metalColor);
    query.bindValue(":sizeNo", order.sizeNo);
    query.bindValue(":sizeMM", order.sizeMM);
    query.bindValue(":length", order.length);
    query.bindValue(":width", order.width);
    query.bindValue(":height", order.height);
    query.bindValue(":diaPacific", order.diaPacific);
    query.bindValue(":diaPurity", order.diaPurity);
    query.bindValue(":diaColor", order.diaColor);
    query.bindValue(":diaPrice", order.diaPrice);
    query.bindValue(":stPacific", order.stPacific);
    query.bindValue(":stPurity", order.stPurity);
    query.bindValue(":stColor", order.stColor);
    query.bindValue(":stPrice", order.stPrice);
    query.bindValue(":designNo1", order.designNo1);
    query.bindValue(":designNo2", order.designNo2);
    query.bindValue(":image1Path", order.image1Path);
    query.bindValue(":image2Path", order.image2Path);
    query.bindValue(":metalCertiName", order.metalCertiName);
    query.bindValue(":metalCertiType", order.metalCertiType); // ✅ fixed typo
    query.bindValue(":diaCertiName", order.diaCertiName);
    query.bindValue(":diaCertiType", order.diaCertiType);     // ✅ fixed typo
    query.bindValue(":pesSaki", order.pesSaki);
    query.bindValue(":chainLock", order.chainLock);
    query.bindValue(":polish", order.polish);
    query.bindValue(":settingLabour", order.settingLabour);
    query.bindValue(":metalStemp", order.metalStemp);
    query.bindValue(":paymentMethod", order.paymentMethod);
    query.bindValue(":totalAmount", order.totalAmount);
    query.bindValue(":advance", order.advance);
    query.bindValue(":remaining", order.remaining);
    query.bindValue(":note", order.note);
    query.bindValue(":extraDetail", order.extraDetail);
    query.bindValue(":id", order.id);

    bool success = query.exec();
    if (!success) {
        qDebug() << "❌ Update failed:" << query.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("save_order_conn");
        return false;
    }

    // Only insert into Order-Status if update succeeded
    QSqlQuery addStatus(db);
    addStatus.prepare(R"(INSERT INTO "Order-Status" (jobNo) VALUES (:jobNo))");
    addStatus.bindValue(":jobNo", order.jobNo);
    if (!addStatus.exec()) {
        qDebug() << "❌ Failed to insert into Order-Status:" << addStatus.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("save_order_conn");
        return false;
    }

    if (!db.commit()) {
        qDebug() << "❌ Commit failed:" << db.lastError().text();
        db.rollback();
        db.close();
        QSqlDatabase::removeDatabase("save_order_conn");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("save_order_conn");
    return true;
}

QList<QVariantList> DatabaseUtils:: fetchOrderListDetails() {
    QList<QVariantList> orderList;

    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/mega_mine_orderbook.db");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "order_list");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ Database not open: " << db.lastError().text();
        return orderList;
    }

    {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT
                od.sellerId, od.partyId, os.jobNo,
                os.Manager, os.Designer, os.Manufacturer, os.Accountant,
                od.orderDate, od.deliveryDate, od.image1Path,
                os.Order_Approve, os.Design_Approve, os.Quality_Approve,
                os.Order_Note, os.Design_Note, os.Quality_Note
            FROM "OrderBook-Detail" od
            LEFT JOIN "Order-Status" os ON od.jobNo = os.jobNo
        )");

        if (!query.exec()) {
            qDebug() << "❌ Error executing query:" << query.lastError().text();
            db.close();
            QSqlDatabase::removeDatabase("order_list");
            return orderList;
        }

        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < 16; ++i) {
                row.append(query.value(i));
            }
            orderList.append(row);
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("order_list");
    return orderList;
}

QStringList DatabaseUtils::fetchPartyNamesForUser(const QString &userId)
{
    QStringList partyList;
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/luxeMineAuthentication.db");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fetch_party_conn");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ Database connection failed for user:" << userId << "Error:" << db.lastError().text();
        return partyList;
    }

    {
        QSqlQuery query(db);
        query.prepare("SELECT name, id FROM Partys WHERE userId = :uid");
        query.bindValue(":uid", userId);

        if (!query.exec()) {
            qDebug() << "❌ Query failed for user:" << userId << "Error:" << query.lastError().text();
        } else {
            partyList.append("-");
            while (query.next()) {
                QString name = query.value(0).toString();
                QString id = query.value(1).toString();
                QString displayText = QString("%1 (%2)").arg(name, id);
                partyList.append(displayText);
            }
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("fetch_party_conn");
    return partyList;
}

bool DatabaseUtils::insertParty(const PartyData &party)
{
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/luxeMineAuthentication.db");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "insert_party_conn");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB for party insert:" << db.lastError().text();
        return false;
    }

    bool success = false;
    {
        QSqlQuery query(db);
        query.prepare(R"(
            INSERT INTO Partys (id, name, email, mobileNo, address, city, state, country, areaCode, userId, date)
            VALUES (:id, :name, :email, :mobileNo, :address, :city, :state, :country, :areaCode, :userId, :date)
        )");

        query.bindValue(":id", party.id);
        query.bindValue(":name", party.name);
        query.bindValue(":email", party.email);
        query.bindValue(":mobileNo", party.mobileNo);
        query.bindValue(":address", party.address);
        query.bindValue(":city", party.city);
        query.bindValue(":state", party.state);
        query.bindValue(":country", party.country);
        query.bindValue(":areaCode", party.areaCode);
        query.bindValue(":userId", party.userId);
        query.bindValue(":date", party.date);

        success = query.exec();
        if (!success)
            qDebug() << "❌ Failed to insert party:" << query.lastError().text();
    }

    db.close();
    QSqlDatabase::removeDatabase("insert_party_conn");
    return success;
}

PartyInfo DatabaseUtils::fetchPartyDetails(const QString &userId, const QString &partyId)
{
    PartyInfo info;
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/luxeMineAuthentication.db");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fetch_party_detail_conn");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ Failed to open DB in fetchPartyDetails:" << db.lastError().text();
        return info;
    }

    {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT id, name, address, city, state, country
            FROM Partys
            WHERE userId = :uid AND id = :pid
            LIMIT 1
        )");
        query.bindValue(":uid", userId);
        query.bindValue(":pid", partyId);

        if (query.exec() && query.next()) {
            info.id      = query.value("id").toString();
            info.name    = query.value("name").toString();
            info.address = query.value("address").toString();
            info.city    = query.value("city").toString();
            info.state   = query.value("state").toString();
            info.country = query.value("country").toString();
        } else {
            qDebug() << "❌ Query failed or no result in fetchPartyDetails:" << query.lastError().text();
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("fetch_party_detail_conn");
    return info;
}

LoginResult DatabaseUtils::authenticateUser(const QString &userId, const QString &password)
{
    LoginResult result;
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("database/luxeMineAuthentication.db");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "auth_conn");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "❌ Failed to open authentication DB:" << db.lastError().text();
        return result;
    }

    {
        QSqlQuery query(db);
        query.prepare("SELECT userName, role FROM OrderBook_Login WHERE userId = :id AND password = :pwd");
        query.bindValue(":id", userId);
        query.bindValue(":pwd", password);

        if (query.exec() && query.next()) {
            result.success = true;
            result.userName = query.value("userName").toString();
            result.role = query.value("role").toString();
        } else if (query.lastError().isValid()) {
            qDebug() << "❌ Login query error:" << query.lastError().text();
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("auth_conn");
    return result;
}


