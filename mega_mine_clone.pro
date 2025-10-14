QT       += core gui sql printsupport

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
    commontypes.cpp \
    databaseutils.cpp \
    diamonissueretbro.cpp \
    imageclicklabel.cpp \
    jewelrymenu.cpp \
    jobsheet.cpp \
    loginwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    managegold.cpp \
    managegoldreturn.cpp \
    # modifycatalogdialog.cpp \
    orderlist.cpp \
    ordermenu.cpp \
    pch.cpp \
    pdflistdialog.cpp \
    pdfutils.cpp \
    readonlydelegate.cpp \
    user.cpp \
    utils.cpp

HEADERS += \
    addcatalog.h \
    admin.h \
    adminmenubuttons.h \
    cartitemwidget.h \
    commontypes.h \
    databaseutils.h \
    diamonissueretbro.h \
    imageclicklabel.h \
    jewelrymenu.h \
    jobsheet.h \
    loginwindow.h \
    mainwindow.h \
    managegold.h \
    managegoldreturn.h \
    # modifycatalogdialog.h \
    orderlist.h \
    ordermenu.h \
    pch.h \
    pdflistdialog.h \
    pdfutils.h \
    readonlydelegate.h \
    user.h \
    utils.h

FORMS += \
    addcatalog.ui \
    admin.ui \
    adminmenubuttons.ui \
    diamonissueretbro.ui \
    jobsheet.ui \
    loginwindow.ui \
    mainwindow.ui \
    managegold.ui \
    managegoldreturn.ui \
    # modifycatalogdialog.ui \
    orderlist.ui \
    ordermenu.ui \
    pdflistdialog.ui \
    user.ui


QXLSX_PARENTPATH=./         # current QXlsx path is . (. means curret directory)
QXLSX_HEADERPATH=./header/  # current QXlsx header path is ./header/
QXLSX_SOURCEPATH=./source/  # current QXlsx source path is ./source/
include(./QXlsx.pri)
# INCLUDEPATH = ./include
# include(./qtcsv.pri)


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resource.qrc

DISTFILES +=

SUBDIRS += \
    qtcsv.pro
