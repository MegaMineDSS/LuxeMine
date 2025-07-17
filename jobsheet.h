#ifndef JOBSHEET_H
#define JOBSHEET_H

#include <QDialog>
#include <QKeyEvent>

namespace Ui {
class JobSheet;
}

class JobSheet : public QDialog
{
    Q_OBJECT

public:
    explicit JobSheet(QWidget *parent = nullptr);
    ~JobSheet();

protected:
    void keyPressEvent(QKeyEvent *event) override;


private:
    void addTableRow(QTableWidget *table);
    Ui::JobSheet *ui;
};

#endif // JOBSHEET_H
