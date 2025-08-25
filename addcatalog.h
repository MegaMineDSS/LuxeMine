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
    ~AddCatalog() override;

private slots:
    void on_save_insert_clicked();
    void on_brows_clicked();
    void on_goldTable_cellChanged(int row, int column);
    void onJewelryItemSelected(const QString &item);
    void on_addCatalog_cancel_button_clicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupGoldTable();
    void calculateGoldWeights(QTableWidgetItem *item);
    void addTableRow(QTableWidget *table, const QString &tableType);

    Ui::AddCatalog *ui {nullptr};   // UI owned by this dialog
    JewelryMenu *jewelryMenu {nullptr}; // Owned by Qt parent (AddCatalog)
    QString selectedImageType;
};

#endif // ADDCATALOG_H
