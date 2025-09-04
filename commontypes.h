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

struct PartyData {
    QString id;
    QString name;
    QString email;
    int mobileNo;
    QString address;
    QString city;
    QString state;
    QString country;
    int areaCode;
    QString userId;
    QString date;
};

struct PartyInfo {
    QString id;
    QString name;
    QString address;
    QString city;
    QString state;
    QString country;
};

struct LoginResult {
    bool success = false;
    QString userName;
    QString role;
};

struct JobSheetRequest {
    QString sellerId;
    QString partyId;
    QString jobNo;
    QString manager;
    QString designer;
    QString manufacturer;
    QString accountant;
    int requestId;
    QString requestRole;
    QString requestRoleId;
    QString fromStatus;
    QString toStatus;
    QString requestTime;
};

struct JobSheetData {
    QString sellerId;
    QString partyId;
    QString jobNo;
    QString orderNo;
    QString clientId;
    QString orderDate;
    QString deliveryDate;

    int productPis = 0;   // numeric
    QString designNo;

    QString metalPurity;
    QString metalColor;

    double sizeNo   = 0.0;
    double sizeMM   = 0.0;
    double length   = 0.0;
    double width    = 0.0;
    double height   = 0.0;

    QString imagePath;
};


struct StoneData {
    QString type;
    QString quantity;
    QString sizeMM;
};



#endif // COMMONTYPES_H
