#ifndef ADDCATALOG_H
#define ADDCATALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QStandardItem>
#include <QListView>

#include "jewelrymenu.h"

class QKeyEvent;

namespace Ui {
class Catalog;
}

class AddCatalog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCatalog(QWidget *parent = nullptr);
    ~AddCatalog() override;

private slots:
    void on_save_insert_clicked();
    void on_brows_clicked();
    void on_goldTable_cellChanged(int row, int column);
    void onJewelryItemSelected(const QString &item);
    void on_addCatalog_cancel_button_clicked();
    void on_bulk_import_button_released();


    void on_add_catalog_button_released();

    void on_modify_catalog_button_released();

    void on_delete_catalog_button_released();

    void loadCatalogForModify();

    void onModifyCatalogContextMenuRightClicked(const QPoint &pos) ;

    void closeEvent(QCloseEvent *event) override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupGoldTable();
    void calculateGoldWeights(QTableWidgetItem *item);
    void addTableRow(QTableWidget *table, const QString &tableType);
    void setupModifyCatalogView();
    void loadModifyCatalogData();
    void modifyClickedAction(const QString &designNo) ;
    void deleteClickedAction(const QString &designNo) ;
    void cancelModifyMode() ;
    void resetAddCatalogUI() ;




    Ui::Catalog *ui {nullptr};   // UI owned by this dialog
    JewelryMenu *jewelryMenu {nullptr}; // Owned by Qt parent (AddCatalog)
    QString selectedImageType;
    QListView *modifyCatalogView {nullptr} ;
    QStandardItemModel *modifyCatalogModel {nullptr} ;
    QSqlDatabase modifyCatalogConn;

    QHash <QString, QStandardItem*> modifyItemMap ;
    bool isModifyMode = false ;
    bool deleteIsSet = false ;
};

#endif // ADDCATALOG_H
