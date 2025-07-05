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

struct OrderData {
    int id;
    QString sellerName, sellerId, partyId, partyName, jobNo, orderNo;
    QString clientId, agencyId, shopId, retailleId, starId;
    QString address, city, state, country;
    QString orderDate, deliveryDate;

    QString productName, productPis, approxProductWt, metalPrice;
    QString metalName, metalPurity, metalColor;

    QString sizeNo, sizeMM, length, width, height;

    QString diaPacific, diaPurity, diaColor, diaPrice;
    QString stPacific, stPurity, stColor, stPrice;

    QString designNo1, designNo2;
    QString image1Path, image2Path;

    QString metalCertiName, metalCertiType;
    QString diaCertiName, diaCertiType;

    QString pesSaki, chainLock, polish;
    QString settingLabour, metalStemp, paymentMethod;

    QString totalAmount, advance, remaining;
    QString note, extraDetail;
};


#endif // COMMONTYPES_H
