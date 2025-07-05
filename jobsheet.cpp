#include "jobsheet.h"
#include "ui_jobsheet.h"

JobSheet::JobSheet(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::JobSheet)
{
    ui->setupUi(this);
}

JobSheet::~JobSheet()
{
    delete ui;
}
