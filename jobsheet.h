#ifndef JOBSHEET_H
#define JOBSHEET_H

#include <QDialog>
#include <QKeyEvent>

#include "managegold.h"
#include "managegoldreturn.h"

namespace Ui {
class JobSheet;
}

class JobSheet : public QDialog
{
    Q_OBJECT

public:
    explicit JobSheet(QWidget *parent = nullptr, const QString &jobNo = QString(), const QString &role = QString());

    ~JobSheet();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;


private:
    void addTableRow(QTableWidget *table);
    void set_value(const QString &jobNo);
    void loadImageForDesignNo();
    void saveDesignNoAndImagePath(const QString &designNo, const QString &imagePath);

    void set_value_designer();
    void set_value_manuf();

    void updateGoldTotalWeight();
    void handleCellSave(int row, int col);

private slots:
        void onGoldDetailCellClicked(QTableWidgetItem *item); // New slot for cell click


private:
    Ui::JobSheet *ui;
    QString userRole;
    int finalWidth = 0;
    int finalHeight = 0;


    QPixmap originalPixmap;


    ManageGold *newManageGold = nullptr;
    bool manageGold = false;

    ManageGoldReturn *newManageGoldReturn = nullptr;
    bool manageGoldReturn = false;




};

#endif // JOBSHEET_H
