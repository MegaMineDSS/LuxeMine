#ifndef MANAGEGOLDRETURN_H
#define MANAGEGOLDRETURN_H

#include <QDialog>

namespace Ui {
class ManageGoldReturn;
}

class ManageGoldReturn : public QDialog
{
    Q_OBJECT


signals:
    void menuHidden();
    void totalWeightCalculated(double weight);


public:
    explicit ManageGoldReturn(QWidget *parent = nullptr);
    ~ManageGoldReturn();

private slots:
    void on_returnPushButton_clicked();

    void on_addPushButton_clicked();

private:
    void hideEvent(QHideEvent *event);
    void loadFillingIssueHistory();


private:
    Ui::ManageGoldReturn *ui;
};

#endif // MANAGEGOLDRETURN_H
