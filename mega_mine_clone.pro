QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addcatalog.cpp \
    admin.cpp \
    cartitemwidget.cpp \
    databaseutils.cpp \
    imageclicklabel.cpp \
    jewelrymenu.cpp \
    main.cpp \
    mainwindow.cpp \
    orderlist.cpp \
    ordermenu.cpp \
    pdflistdialog.cpp \
    pdfutils.cpp \
    user.cpp\
    databasemanager.cpp

HEADERS += \
    CommonTypes.h \
    DatabaseUtils.h \
    ReadOnlyDelegate.h \
    Utils.h \
    addcatalog.h \
    admin.h \
    cartitemwidget.h \
    imageclicklabel.h \
    jewelrymenu.h \
    mainwindow.h \
    orderlist.h \
    ordermenu.h \
    pdflistdialog.h \
    pdfutils.h \
    user.h\
    databasemanager.h

FORMS += \
    addcatalog.ui \
    admin.ui \
    mainwindow.ui \
    orderlist.ui \
    ordermenu.ui \
    pdflistdialog.ui \
    user.ui


# DEFINES += QUAZIP_STATIC

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resource.qrc
