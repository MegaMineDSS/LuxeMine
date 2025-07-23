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

    Ui::JobSheet *ui;
    QString userRole;
    int finalWidth = 0;
    int finalHeight = 0;




};

#endif // JOBSHEET_H
