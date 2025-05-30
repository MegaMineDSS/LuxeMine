#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QPixmap>
#include <QResizeEvent>
#include <QPointer>
#include "admin.h"
#include "user.h"
#include "addcatalog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;
    Admin *newAdmin = nullptr;
    User *newUser = nullptr;
    AddCatalog *newAddCatalog = nullptr;
    void setRandomBackground();

};

#endif // MAINWINDOW_H
