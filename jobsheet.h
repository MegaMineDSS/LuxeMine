#ifndef JOBSHEET_H
#define JOBSHEET_H

#include <QDialog>

namespace Ui {
class JobSheet;
}

class JobSheet : public QDialog
{
    Q_OBJECT

public:
    explicit JobSheet(QWidget *parent = nullptr);
    ~JobSheet();

private:
    Ui::JobSheet *ui;
};

#endif // JOBSHEET_H
