#ifndef MANAGEGOLD_H
#define MANAGEGOLD_H

#include <QDialog>

namespace Ui {
class ManageGold;
}

class ManageGold : public QDialog
{
    Q_OBJECT

public:
    explicit ManageGold(QWidget *parent = nullptr);
    ~ManageGold();

signals:
    void menuHidden();
    void totalWeightCalculated(double weight);

private slots:
    void on_pushButton_clicked();

    void on_issueAddPushButton_clicked();

private:
    void hideEvent(QHideEvent *event);
    void loadFillingIssueHistory();


private:
    Ui::ManageGold *ui;
};

#endif // MANAGEGOLD_H
