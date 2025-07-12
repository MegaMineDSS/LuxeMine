#include "pdflistdialog.h"
#include "ui_pdflistdialog.h"

#include <QTableWidgetItem>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

PdfListDialog::PdfListDialog(const QString &userId, const QList<PdfRecord> &pdfRecords, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PdfListDialog)
    ,userId(userId)
{
    ui->setupUi(this);

    setWindowTitle(QString("PDFs for User: %1").arg(userId));

    // Setup table
    ui->pdfTableWidget->setColumnCount(3);
    ui->pdfTableWidget->setHorizontalHeaderLabels({"PDF Path", "Creation Time", "Last PDF"});
    ui->pdfTableWidget->setRowCount(pdfRecords.size());
    ui->pdfTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->pdfTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Populate table
    for (int i = 0; i < pdfRecords.size(); ++i) {
        const PdfRecord &record = pdfRecords[i];
        QTableWidgetItem *pathItem = new QTableWidgetItem(record.pdf_path);
        QTableWidgetItem *timeItem = new QTableWidgetItem(record.time);
        QTableWidgetItem *lastPdfItem = new QTableWidgetItem(i == 0 ? "Latest" : "");

        // Store pdf_path in UserRole for opening
        pathItem->setData(Qt::UserRole, record.pdf_path);
        lastPdfItem->setData(Qt::UserRole, record.pdf_path);

        ui->pdfTableWidget->setItem(i, 0, pathItem);
        ui->pdfTableWidget->setItem(i, 1, timeItem);
        ui->pdfTableWidget->setItem(i, 2, lastPdfItem);
    }

    ui->pdfTableWidget->resizeColumnsToContents();

}

PdfListDialog::~PdfListDialog()
{
    delete ui;
}

void PdfListDialog::on_pdfTableWidget_cellDoubleClicked(int row, int column)
{
    if (column == 0 || column == 2) { // PDF Path or Last PDF column
        QTableWidgetItem *item = ui->pdfTableWidget->item(row, column);
        QString pdfPath = item->data(Qt::UserRole).toString();
        QFile pdfFile(pdfPath);
        if (!pdfFile.exists()) {
            QMessageBox::warning(this, "Error", "PDF file does not exist: " + pdfPath);
            return;
        }

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath))) {
            QMessageBox::critical(this, "Error", "Failed to open PDF: " + pdfPath);
        } else {
            qDebug() << "Opened PDF:" << pdfPath;
        }
    }
}
