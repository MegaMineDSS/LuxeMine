#ifndef PDFLISTDIALOG_H
#define PDFLISTDIALOG_H

#include <QDialog>
#include <QList>
#include "CommonTypes.h"

namespace Ui {
class PdfListDialog;
}

class PdfListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PdfListDialog(const QString &userId, const QList<PdfRecord> &pdfRecords, QWidget *parent = nullptr);
    ~PdfListDialog();

private slots:
    void on_pdfTableWidget_cellDoubleClicked(int row, int column);

private:
    Ui::PdfListDialog *ui;
    QString userId;
};

#endif // PDFLISTDIALOG_H
