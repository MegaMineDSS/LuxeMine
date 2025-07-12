#ifndef ADDCATALOG_H
#define ADDCATALOG_H

#include <QDialog>
#include <QTableWidget>

#include "jewelrymenu.h"

class QKeyEvent;

namespace Ui {
class AddCatalog;
}

class AddCatalog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCatalog(QWidget *parent = nullptr);
    ~AddCatalog();

private slots:
    void on_save_insert_clicked();
    void on_brows_clicked();
    void on_goldTable_cellChanged(int row, int column);
    void onJewelryItemSelected(const QString &item); // Handle selection from JewelryMenu


    void on_addCatalog_cancel_button_clicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupGoldTable();
    void calculateGoldWeights(QTableWidgetItem *item);
    void addTableRow(QTableWidget *table, const QString &tableType);
    JewelryMenu *jewelryMenu; // Instance of the new class
    QString selectedImageType; // Store the selected item

private:
    Ui::AddCatalog *ui;
};

#endif // ADDCATALOG_H
