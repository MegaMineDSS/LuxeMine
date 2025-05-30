#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <QString>

// Struct to hold selection data for cart items
struct SelectionData {
    int imageId;
    QString goldType;
    int itemCount;
    QString stoneJson;
    QString diamondJson;
    QString pdf_path;
};

// Struct to hold PDF record data
struct PdfRecord {
    QString pdf_path;
    QString time;
};

#endif // COMMONTYPES_H
