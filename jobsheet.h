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
    explicit JobSheet(QWidget *parent = nullptr, const QString &jobNo = QString());

    ~JobSheet();

protected:
    void keyPressEvent(QKeyEvent *event) override;


private:
    void addTableRow(QTableWidget *table);
    void set_value(const QString &jobNo);
    void loadImageForDesignNo();
    void saveDesignNoAndImagePath(const QString &designNo, const QString &imagePath);
    Ui::JobSheet *ui;
};

#endif // JOBSHEET_H
