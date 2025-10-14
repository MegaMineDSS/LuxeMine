#ifndef DIAMONISSUERETBRO_H
#define DIAMONISSUERETBRO_H

#include <QDialog>

namespace Ui {
class DiamonIssueRetBro;
}

class DiamonIssueRetBro : public QDialog
{
    Q_OBJECT

public:
    explicit DiamonIssueRetBro(QWidget *parent = nullptr);
    ~DiamonIssueRetBro();
    void setContext(int row, int col, const QString &jobNo); // from JobSheet

signals:
    void menuHidden();
    void valuesUpdated(int row, const QVariantMap &vals);

private slots:
    void onRadioChanged();
    void onTypeChanged();
    void onSaveClicked();
    // void onCancelClicked();

    void on_pushButton_clicked();

private:
    void hideEvent(QHideEvent *event);

private:
    Ui::DiamonIssueRetBro *ui;
    int currentRow;
    int currentCol;
    QString currentJobNo;
    QString currentMode;  // "issue" or "return"


    void loadTypeOptions();
    void loadSizeOptions(const QString &type);
    void saveToDatabase(const QJsonObject &entry);
    void loadHistoryForCurrentContext();
};
#endif // DIAMONISSUERETBRO_H
