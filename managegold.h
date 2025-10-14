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
    enum Mode {
        Filling,
        Returning,
        Dust
    };

    explicit ManageGold(QWidget *parent = nullptr);
    ~ManageGold();

    void setMode(Mode mode);
    Mode currentMode;  // new

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;


signals:
    void menuHidden();
    void totalWeightCalculated(double weight);

private slots:
    void on_pushButton_clicked();
    void on_issueAddPushButton_clicked();

private:
    void hideEvent(QHideEvent *event);
    void loadHistory();  // renamed generic version

private:
    Ui::ManageGold *ui;

};

#endif // MANAGEGOLD_H
