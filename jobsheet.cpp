#include "jobsheet.h"
#include "ui_jobsheet.h"



JobSheet::JobSheet(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::JobSheet)
{
    ui->setupUi(this);

    ui->extraNoteTextEdit->setText(R"(ACKNOWLEDGMENT OF ENTRUSTMENT
We Hereby acknowledge receipt of the following goods mentioned overleaf which you have entrusted to melus and to which IAve hold in trust for you for the following purpose and on following conditions. (1) The goods have been entrusted to me/us for the sale purpose of being shown to intending purchasers of inspection.(2) The Goods remain your property and Iwe acquire no right to property or interest in them till sale note signed by you is passed or till the price is paid in respect there of not with standing the fact that mention is made of the rate or price in the particulars of goods herein behind set.( 3) IWe agree not to sell or pledge, or montage or hypothecate the said goods or otherwise dea; with them in any manner till a sale note signed by you is passed or the price is paid to you. (4) The goods are to be returned to you forthwith whenever demanded back. (5) The Goods will be at my/out risk in all respect till a sale note signed by you is passed in respecte there of or ti the price is paid to you or till the goods are returned to you and Awe am/are responsible to you for the retum of the said goods in the same condition as I/we have received the same. (6) Subject to Surat Jurisdiction.)");

    ui->extraNoteTextEdit->setStyleSheet("font-size: 8pt;");


}

JobSheet::~JobSheet()
{
    delete ui;
}


void JobSheet::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if(ui->diamondAndStoneDetailTableWidget->hasFocus()){
            addTableRow(ui->diamondAndStoneDetailTableWidget);
        }
    }
    else
    {
        QDialog::keyPressEvent(event);
    }
}

void JobSheet::addTableRow(QTableWidget *table)
{
    int newRow = table->rowCount();
    table->insertRow(newRow);
}
