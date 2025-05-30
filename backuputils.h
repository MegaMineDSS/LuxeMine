#ifndef BACKUPUTILS_H
#define BACKUPUTILS_H

#include <QString>

class BackupUtils
{
public:
    BackupUtils() = delete; // Prevent instantiation, all methods are static
    static bool exportAdminBackup(const QString &zipPath, const QString &dbPath = "database/mega_mine_image.db", const QString &imagesDir = "images");
    static bool importAdminBackup(const QString &zipPath, const QString &dbPath = "database/mega_mine_image.db", const QString &imagesDir = "images");
    static bool exportUserBackup(const QString &filePath, const QString &userId, const QString &dbPath = "database/mega_mine_image.db", const QString &pdfsDir = "pdfs");
    static bool importUserBackup(const QString &filePath, const QString &dbPath = "database/mega_mine_image.db", const QString &pdfsDir = "pdfs");
};

#endif // BACKUPUTILS_H
