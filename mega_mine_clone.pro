QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20 precompile_header

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


PRECOMPILED_HEADER = pch.h


SOURCES += \
    addcatalog.cpp \
    admin.cpp \
    adminmenubuttons.cpp \
    cartitemwidget.cpp \
    databaseutils.cpp \
    imageclicklabel.cpp \
    jewelrymenu.cpp \
    jobsheet.cpp \
    loginwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    orderlist.cpp \
    ordermenu.cpp \
    pch.cpp \
    pdflistdialog.cpp \
    pdfutils.cpp \
    user.cpp

HEADERS += \
    CommonTypes.h \
    DatabaseUtils.h \
    ReadOnlyDelegate.h \
    Utils.h \
    addcatalog.h \
    admin.h \
    adminmenubuttons.h \
    cartitemwidget.h \
    imageclicklabel.h \
    jewelrymenu.h \
    jobsheet.h \
    loginwindow.h \
    mainwindow.h \
    orderlist.h \
    ordermenu.h \
    pch.h \
    pdflistdialog.h \
    pdfutils.h \
    user.h

FORMS += \
    addcatalog.ui \
    admin.ui \
    adminmenubuttons.ui \
    jobsheet.ui \
    loginwindow.ui \
    mainwindow.ui \
    orderlist.ui \
    ordermenu.ui \
    pdflistdialog.ui \
    user.ui



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resource.qrc
