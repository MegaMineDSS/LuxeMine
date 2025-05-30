#ifndef PDFUTILS_H
#define PDFUTILS_H
#include <QString>
#include <QWidget>
#include <QTableWidget>
#include "CommonTypes.h"

class PdfUtils
{
public:
    PdfUtils();
    static bool generateCartPdf(const QString &pdfPath, const QString &userId, const QString &userName,
                                const QString &companyName, const QString &mobileNo,
                                const QList<SelectionData> &selections, QWidget *cartItemsContainer,
                                QTableWidget *diamondTable, QTableWidget *stoneTable, const QString &goldWeightText);
};

#endif // PDFUTILS_H
