#include "backuputils.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDir>
#include <QFile>
#include <QSqlRecord>
#include <QFileInfo>
#include <QSqlError>
// #include <quazip.h>
// #include <quazipfile.h>
#include "D:\quazip-master\quazip\quazip.h"
#include "D:\quazip-master\quazip\quazipfile.h"
#include <QUuid>


namespace {
const QStringList ADMIN_TABLES = {"Fancy_diamond", "Gold_Price", "Round_diamond", "image_data", "jewelry_menu", "stones"};
const QStringList USER_TABLES = {"users", "user_cart"};

// Helper to generate a unique connection name
QString uniqueConnectionName() {
    return "backuputils_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// Helper to copy table schema and data
bool copyTableData(QSqlDatabase &srcDb, QSqlDatabase &dstDb, const QString &table, const QString &userId = QString()) {
    QSqlQuery srcQuery(srcDb);
    QSqlQuery dstQuery(dstDb);

    // Check if table exists in source database
    if (!srcQuery.exec(QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(table))) {
        qDebug() << "Failed to check table existence for" << table << ":" << srcQuery.lastError();
        return false;
    }
    if (!srcQuery.next()) {
        qDebug() << "Table" << table << "does not exist in source database";
        return false;
    }

    // Check if table exists in destination database
    bool tableExistsInDst = false;
    if (!dstQuery.exec(QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(table))) {
        qDebug() << "Failed to check table existence in destination for" << table << ":" << dstQuery.lastError();
        return false;
    }
    if (dstQuery.next()) {
        tableExistsInDst = true;
        qDebug() << "Table" << table << "already exists in destination database, skipping creation";
    }

    // Copy table schema if it doesn't exist in the destination
    if (!tableExistsInDst) {
        if (!srcQuery.exec(QString("SELECT sql FROM sqlite_master WHERE type='table' AND name='%1'").arg(table))) {
            qDebug() << "Failed to retrieve schema for table" << table << ":" << srcQuery.lastError();
            return false;
        }
        if (srcQuery.next()) {
            QString createTableSql = srcQuery.value(0).toString();
            if (!dstQuery.exec(createTableSql)) {
                qDebug() << "Failed to create table" << table << "in destination database:" << dstQuery.lastError();
                return false;
            }
        } else {
            qDebug() << "No schema found for table" << table;
            return false;
        }
    }

    // Prepare data query
    QString selectQuery = QString("SELECT * FROM %1").arg(table);
    if (!userId.isEmpty()) {
        selectQuery += QString(" WHERE user_id = '%1'").arg(userId);
    }
    if (!srcQuery.exec(selectQuery)) {
        qDebug() << "Failed to select data from" << table << ":" << srcQuery.lastError();
        return false;
    }

    // Check if there is data to copy
    int rowCount = 0;
    while (srcQuery.next()) {
        rowCount++;
        QStringList columns;
        QStringList placeholders;
        QSqlRecord record = srcQuery.record();
        for (int i = 0; i < record.count(); ++i) {
            columns << record.fieldName(i);
            placeholders << "?";
        }
        QString insertQuery = QString("INSERT INTO %1 (%2) VALUES (%3)")
                                  .arg(table, columns.join(","), placeholders.join(","));
        dstQuery.prepare(insertQuery);
        for (int i = 0; i < record.count(); ++i) {
            dstQuery.addBindValue(srcQuery.value(i));
        }
        if (!dstQuery.exec()) {
            qDebug() << "Failed to insert data into" << table << ":" << dstQuery.lastError();
            return false;
        }
    }
    qDebug() << "Copied" << rowCount << "rows from table" << table;
    return true;
}

// Helper to add files to zip
bool addFilesToZip(QuaZip &zip, const QString &dirPath, const QString &zipDir) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        qDebug() << "Directory does not exist:" << dirPath;
        return true; // No directory, no files to add
    }

    QuaZipFile zipFile(&zip);
    for (const QFileInfo &fileInfo : dir.entryInfoList(QDir::Files)) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open file for zipping:" << fileInfo.fileName() << ":" << file.errorString();
            continue;
        }
        QString zipPath = QString("%1/%2").arg(zipDir, fileInfo.fileName());
        if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(zipPath))) {
            qDebug() << "Failed to add file to zip:" << fileInfo.fileName();
            file.close();
            continue;
        }
        zipFile.write(file.readAll());
        zipFile.close();
        file.close();
        qDebug() << "Added file to zip:" << zipPath;
    }
    return true;
}

// Helper to extract files from zip
bool extractFilesFromZip(QuaZip &zip, const QString &outputDir, const QString &prefix) {
    QDir().mkpath(outputDir);
    QuaZipFile zipFile(&zip);
    zip.goToFirstFile();
    do {
        QString fileName = zip.getCurrentFileName();
        if (fileName.startsWith(prefix)) {
            if (!zipFile.open(QIODevice::ReadOnly)) {
                qDebug() << "Failed to read file from zip:" << fileName;
                continue;
            }
            QString outPath = QString("%1/%2").arg(outputDir, fileName.mid(prefix.length()));
            QFile file(outPath);
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "Failed to write extracted file:" << outPath << ":" << file.errorString();
                zipFile.close();
                continue;
            }
            file.write(zipFile.readAll());
            file.close();
            zipFile.close();
            qDebug() << "Extracted file:" << outPath;
        }
    } while (zip.goToNextFile());
    return true;
}
}

bool BackupUtils::exportAdminBackup(const QString &zipPath, const QString &dbPath, const QString &imagesDir)
{
    // Create temporary database
    QString tempDbPath = QDir::tempPath() + "/admin_backup.sqlite";
    QFile::remove(tempDbPath); // Ensure clean start

    QSqlDatabase backupDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    backupDb.setDatabaseName(tempDbPath);
    if (!backupDb.open()) {
        qDebug() << "Failed to open backup database at" << tempDbPath << ":" << backupDb.lastError();
        return false;
    }

    QSqlDatabase srcDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    srcDb.setDatabaseName(dbPath);
    if (!srcDb.open()) {
        qDebug() << "Failed to open source database at" << dbPath << ":" << srcDb.lastError();
        backupDb.close();
        QSqlDatabase::removeDatabase(backupDb.connectionName());
        return false;
    }

    // Copy admin tables
    for (const QString &table : ADMIN_TABLES) {
        if (!copyTableData(srcDb, backupDb, table)) {
            qDebug() << "Failed to copy admin table" << table;
            backupDb.close();
            srcDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(srcDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
    }

    backupDb.close();
    srcDb.close();
    QSqlDatabase::removeDatabase(backupDb.connectionName());
    QSqlDatabase::removeDatabase(srcDb.connectionName());

    // Create zip file
    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdCreate)) {
        qDebug() << "Failed to create zip file:" << zipPath;
        QFile::remove(tempDbPath);
        return false;
    }

    // Add database to zip
    QuaZipFile zipFile(&zip);
    QFile dbFile(tempDbPath);
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open temp database for zipping:" << dbFile.errorString();
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }
    if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("backup.sqlite"))) {
        qDebug() << "Failed to add backup.sqlite to zip";
        dbFile.close();
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }
    zipFile.write(dbFile.readAll());
    zipFile.close();
    dbFile.close();

    // Add images to zip
    if (!addFilesToZip(zip, imagesDir, "images")) {
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }

    zip.close();
    QFile::remove(tempDbPath);
    return true;
}

bool BackupUtils::importAdminBackup(const QString &zipPath, const QString &dbPath, const QString &imagesDir)
{
    // Extract database from zip
    qDebug() << "we are in iAB";
    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdUnzip)) {
        qDebug() << "Failed to open zip file:" << zipPath;
        return false;
    }
    qDebug() << "zip open";

    QString tempDbPath = QDir::tempPath() + "/admin_restore.sqlite";
    QFile::remove(tempDbPath);

    QuaZipFile zipFile(&zip);
    if (zip.goToFirstFile()) {
        do {
            QString fileName = zip.getCurrentFileName();
            if (fileName == "backup.sqlite") {
                if (!zipFile.open(QIODevice::ReadOnly)) {
                    qDebug() << "Failed to read backup.sqlite from zip";
                    zip.close();
                    return false;
                }
                QFile tempFile(tempDbPath);
                if (!tempFile.open(QIODevice::WriteOnly)) {
                    qDebug() << "Failed to write temp database:" << tempFile.errorString();
                    zipFile.close();
                    zip.close();
                    return false;
                }
                tempFile.write(zipFile.readAll());
                tempFile.close();
                zipFile.close();
                break;
            }
        } while (zip.goToNextFile());
    } else {
        zip.close();
        return false;
    }

    // Extract images
    if (!extractFilesFromZip(zip, imagesDir, "images/")) {
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }
    zip.close();

    // Open databases
    QSqlDatabase backupDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    backupDb.setDatabaseName(tempDbPath);
    if (!backupDb.open()) {
        qDebug() << "Failed to open backup database:" << backupDb.lastError();
        QFile::remove(tempDbPath);
        return false;
    }

    QSqlDatabase dstDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    dstDb.setDatabaseName(dbPath);
    if (!dstDb.open()) {
        qDebug() << "Failed to open destination database:" << dstDb.lastError();
        backupDb.close();
        QSqlDatabase::removeDatabase(backupDb.connectionName());
        QFile::remove(tempDbPath);
        return false;
    }

    // Copy tables to destination
    for (const QString &table : ADMIN_TABLES) {
        QSqlQuery dstQuery(dstDb);
        // Check if table exists in destination database before deleting
        if (!dstQuery.exec(QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(table))) {
            qDebug() << "Failed to check table existence in destination for" << table << ":" << dstQuery.lastError();
            backupDb.close();
            dstDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(dstDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        if (dstQuery.next()) {
            // Table exists, clear its data
            if (!dstQuery.exec(QString("DELETE FROM %1").arg(table))) {
                qDebug() << "Failed to delete from" << table << ":" << dstQuery.lastError();
                backupDb.close();
                dstDb.close();
                QSqlDatabase::removeDatabase(backupDb.connectionName());
                QSqlDatabase::removeDatabase(dstDb.connectionName());
                QFile::remove(tempDbPath);
                return false;
            }
            qDebug() << "Cleared existing data from table" << table;
        } else {
            qDebug() << "Table" << table << "does not exist in destination database, it will be created";
        }

        if (!copyTableData(backupDb, dstDb, table)) {
            qDebug() << "Failed to copy table" << table;
            backupDb.close();
            dstDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(dstDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
    }

    backupDb.close();
    dstDb.close();
    QSqlDatabase::removeDatabase(backupDb.connectionName());
    QSqlDatabase::removeDatabase(dstDb.connectionName());
    QFile::remove(tempDbPath);
    return true;
}

bool BackupUtils::exportUserBackup(const QString &filePath, const QString &userId, const QString &dbPath, const QString &pdfsDir)
{
    qDebug() << "Starting exportUserBackup for user:" << userId << "to file:" << filePath;

    // Ensure the filePath has a .zip extension
    QString zipFilePath = filePath;
    if (!zipFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
        zipFilePath = zipFilePath + ".zip";
        qDebug() << "Adjusted file path to include .zip extension:" << zipFilePath;
    }

    // Create temporary database
    QString tempDbPath = QDir::tempPath() + "/user_backup_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
    qDebug() << "Step 1: Creating temporary database at:" << tempDbPath;
    QFile::remove(tempDbPath); // Ensure clean start

    QSqlDatabase backupDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    backupDb.setDatabaseName(tempDbPath);
    qDebug() << "Step 2: Opening backup database";
    if (!backupDb.open()) {
        qDebug() << "Failed to open backup database at" << tempDbPath << ":" << backupDb.lastError();
        return false;
    }

    QSqlDatabase srcDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    srcDb.setDatabaseName(dbPath);
    qDebug() << "Step 3: Opening source database at:" << dbPath;
    if (!srcDb.open()) {
        qDebug() << "Failed to open source database at" << dbPath << ":" << srcDb.lastError();
        backupDb.close();
        QSqlDatabase::removeDatabase(backupDb.connectionName());
        return false;
    }

    // Copy user tables
    qDebug() << "Step 4: Copying user tables";
    for (const QString &table : USER_TABLES) {
        qDebug() << "Copying table:" << table << "for user:" << userId;
        if (!copyTableData(srcDb, backupDb, table, userId)) {
            qDebug() << "Failed to copy table" << table << "for user" << userId;
            backupDb.close();
            srcDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(srcDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
    }

    qDebug() << "Step 5: Closing databases";
    backupDb.close();
    srcDb.close();
    QSqlDatabase::removeDatabase(backupDb.connectionName());
    QSqlDatabase::removeDatabase(srcDb.connectionName());

    // Create zip file
    qDebug() << "Step 6: Creating zip file at:" << zipFilePath;
    QuaZip zip(zipFilePath);
    if (!zip.open(QuaZip::mdCreate)) {
        qDebug() << "Failed to create zip file:" << zipFilePath << "- QuaZip error code:" << zip.getZipError();
        QFile::remove(tempDbPath);
        return false;
    }

    // Add database to zip
    qDebug() << "Step 7: Adding database to zip";
    QuaZipFile zipFile(&zip);
    QFile dbFile(tempDbPath);
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open temp database for zipping:" << dbFile.errorString();
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }
    if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("backup.sqlite"))) {
        qDebug() << "Failed to add backup.sqlite to zip - QuaZip error code:" << zipFile.getZipError();
        dbFile.close();
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }
    zipFile.write(dbFile.readAll());
    zipFile.close();
    dbFile.close();
    qDebug() << "Database added to zip as backup.sqlite";

    // Add PDFs to zip
    qDebug() << "Step 8: Adding PDFs to zip from directory:" << pdfsDir;
    if (!addFilesToZip(zip, pdfsDir, "pdfs")) {
        qDebug() << "Failed to add PDFs to zip";
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }

    qDebug() << "Step 9: Closing zip file";
    zip.close();
    if (zip.getZipError() != ZIP_OK) {
        qDebug() << "Error while closing zip file - QuaZip error code:" << zip.getZipError();
        QFile::remove(tempDbPath);
        return false;
    }

    // Verify the zip file exists
    qDebug() << "Step 10: Verifying zip file";
    QFile zipOutputFile(zipFilePath);
    if (!zipOutputFile.exists()) {
        qDebug() << "Zip file was not created at:" << zipFilePath;
        QFile::remove(tempDbPath);
        return false;
    }
    qDebug() << "Zip file created successfully at:" << zipFilePath << "with size:" << zipOutputFile.size() << "bytes";

    // Remove the temporary database file
    qDebug() << "Step 11: Removing temporary database file";
    if (QFile::exists(tempDbPath)) {
        QFile::remove(tempDbPath);
        qDebug() << "Temporary database file removed:" << tempDbPath;
    } else {
        qDebug() << "Temporary database file not found at:" << tempDbPath;
    }

    qDebug() << "exportUserBackup completed successfully";
    return true;
}

bool BackupUtils::importUserBackup(const QString &filePath, const QString &dbPath, const QString &pdfsDir)
{
    qDebug() << "Starting importUserBackup from file:" << filePath;

    // Extract database from zip
    qDebug() << "Step 1: Opening zip file";
    QuaZip zip(filePath);
    if (!zip.open(QuaZip::mdUnzip)) {
        qDebug() << "Failed to open zip file:" << filePath << "- QuaZip error code:" << zip.getZipError();
        return false;
    }

    QString tempDbPath = QDir::tempPath() + "/user_restore_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".sqlite";
    qDebug() << "Step 2: Creating temporary database at:" << tempDbPath;
    QFile::remove(tempDbPath);

    qDebug() << "Step 3: Extracting backup.sqlite from zip";
    QuaZipFile zipFile(&zip);
    bool dbFound = false;
    if (zip.goToFirstFile()) {
        do {
            QString fileName = zip.getCurrentFileName();
            qDebug() << "Found file in zip:" << fileName;
            if (fileName == "backup.sqlite") {
                if (!zipFile.open(QIODevice::ReadOnly)) {
                    qDebug() << "Failed to read backup.sqlite from zip - QuaZip error code:" << zipFile.getZipError();
                    zip.close();
                    return false;
                }
                QFile tempFile(tempDbPath);
                if (!tempFile.open(QIODevice::WriteOnly)) {
                    qDebug() << "Failed to write temp database:" << tempFile.errorString();
                    zipFile.close();
                    zip.close();
                    return false;
                }
                tempFile.write(zipFile.readAll());
                tempFile.close();
                zipFile.close();
                dbFound = true;
                qDebug() << "Successfully extracted backup.sqlite to:" << tempDbPath;
                break;
            }
        } while (zip.goToNextFile());
    }
    if (!dbFound) {
        qDebug() << "No backup.sqlite found in zip";
        zip.close();
        return false;
    }

    // Extract PDFs
    qDebug() << "Step 4: Extracting PDFs from zip to directory:" << pdfsDir;
    if (!extractFilesFromZip(zip, pdfsDir, "pdfs/")) {
        qDebug() << "Failed to extract PDFs from zip";
        zip.close();
        QFile::remove(tempDbPath);
        return false;
    }
    qDebug() << "Step 5: Closing zip file";
    zip.close();

    // Open backup database
    qDebug() << "Step 6: Opening backup database at:" << tempDbPath;
    QSqlDatabase backupDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    backupDb.setDatabaseName(tempDbPath);
    if (!backupDb.open()) {
        qDebug() << "Failed to open backup database:" << backupDb.lastError();
        QFile::remove(tempDbPath);
        return false;
    }

    // Verify the backup contains user tables
    qDebug() << "Step 7: Verifying user tables in backup database";
    for (const QString &table : USER_TABLES) {
        QSqlQuery checkQuery(backupDb);
        if (!checkQuery.exec(QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(table))) {
            qDebug() << "Failed to check table" << table << "in backup database:" << checkQuery.lastError();
            backupDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        if (!checkQuery.next()) {
            qDebug() << "Backup does not contain required table" << table << "- this might be an admin backup";
            backupDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        qDebug() << "Table" << table << "found in backup database";
    }

    // Open destination database
    qDebug() << "Step 8: Opening destination database at:" << dbPath;
    QSqlDatabase dstDb = QSqlDatabase::addDatabase("QSQLITE", uniqueConnectionName());
    dstDb.setDatabaseName(dbPath);
    if (!dstDb.open()) {
        qDebug() << "Failed to open destination database:" << dstDb.lastError();
        backupDb.close();
        QSqlDatabase::removeDatabase(backupDb.connectionName());
        QFile::remove(tempDbPath);
        return false;
    }

    // Validate schema
    qDebug() << "Step 9: Validating schema between backup and destination databases";
    for (const QString &table : USER_TABLES) {
        QSqlQuery srcSchemaQuery(backupDb);
        QSqlQuery dstSchemaQuery(dstDb);
        if (!srcSchemaQuery.exec(QString("SELECT sql FROM sqlite_master WHERE type='table' AND name='%1'").arg(table)) ||
            !dstSchemaQuery.exec(QString("SELECT sql FROM sqlite_master WHERE type='table' AND name='%1'").arg(table))) {
            qDebug() << "Failed to retrieve schema for table" << table << "- Source:" << srcSchemaQuery.lastError() << "Dest:" << dstSchemaQuery.lastError();
            backupDb.close();
            dstDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(dstDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        if (!srcSchemaQuery.next() || !dstSchemaQuery.next()) {
            qDebug() << "Schema not found for table" << table << "in one of the databases";
            backupDb.close();
            dstDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(dstDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        if (srcSchemaQuery.value(0).toString() != dstSchemaQuery.value(0).toString()) {
            qDebug() << "Schema mismatch for table" << table;
            backupDb.close();
            dstDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(dstDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        qDebug() << "Schema validated for table:" << table;
    }

    // Start transaction for importing
    qDebug() << "Step 10: Starting transaction for destination database";
    if (!dstDb.transaction()) {
        qDebug() << "Failed to start destination transaction:" << dstDb.lastError();
        backupDb.close();
        dstDb.close();
        QSqlDatabase::removeDatabase(backupDb.connectionName());
        QSqlDatabase::removeDatabase(dstDb.connectionName());
        QFile::remove(tempDbPath);
        return false;
    }

    // Copy tables to destination
    qDebug() << "Step 11: Copying tables to destination database";
    for (const QString &table : USER_TABLES) {
        QSqlQuery backupQuery(backupDb);
        if (!backupQuery.exec(QString("SELECT * FROM %1").arg(table))) {
            qDebug() << "Failed to select from" << table << "in backup database:" << backupQuery.lastError();
            dstDb.rollback();
            backupDb.close();
            dstDb.close();
            QSqlDatabase::removeDatabase(backupDb.connectionName());
            QSqlDatabase::removeDatabase(dstDb.connectionName());
            QFile::remove(tempDbPath);
            return false;
        }
        int rowCount = 0;
        while (backupQuery.next()) {
            QSqlQuery dstQuery(dstDb); // Create a new query for each row to avoid lifetime issues
            QStringList columns;
            QStringList placeholders;
            QSqlRecord record = backupQuery.record();
            for (int i = 0; i < record.count(); ++i) {
                columns << record.fieldName(i);
                placeholders << "?";
            }
            QString insertQuery = QString("INSERT OR REPLACE INTO %1 (%2) VALUES (%3)")
                                      .arg(table, columns.join(","), placeholders.join(","));
            dstQuery.prepare(insertQuery);
            for (int i = 0; i < record.count(); ++i) {
                dstQuery.addBindValue(backupQuery.value(i));
            }
            if (!dstQuery.exec()) {
                qDebug() << "Failed to insert into" << table << "in destination database:" << dstQuery.lastError();
                dstDb.rollback();
                backupDb.close();
                dstDb.close();
                QSqlDatabase::removeDatabase(backupDb.connectionName());
                QSqlDatabase::removeDatabase(dstDb.connectionName());
                QFile::remove(tempDbPath);
                return false;
            }
            rowCount++;
        }
        qDebug() << "Copied" << rowCount << "rows to table" << table << "in destination database";
    }

    qDebug() << "Step 12: Committing transaction for destination database";
    if (!dstDb.commit()) {
        qDebug() << "Failed to commit destination transaction:" << dstDb.lastError();
        dstDb.rollback();
        backupDb.close();
        dstDb.close();
        QSqlDatabase::removeDatabase(backupDb.connectionName());
        QSqlDatabase::removeDatabase(dstDb.connectionName());
        QFile::remove(tempDbPath);
        return false;
    }

    qDebug() << "Step 13: Closing databases";
    backupDb.close();
    dstDb.close();
    QSqlDatabase::removeDatabase(backupDb.connectionName());
    QSqlDatabase::removeDatabase(dstDb.connectionName());

    qDebug() << "Step 14: Removing temporary database file";
    if (QFile::exists(tempDbPath)) {
        QFile::remove(tempDbPath);
        qDebug() << "Temporary database file removed:" << tempDbPath;
    } else {
        qDebug() << "Temporary database file not found at:" << tempDbPath;
    }

    qDebug() << "importUserBackup completed successfully";
    return true;
}
