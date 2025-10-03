#include "diamonissueretbro.h"
#include "ui_diamonissueretbro.h"

DiamonIssueRetBro::DiamonIssueRetBro(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DiamonIssueRetBro)
{
    ui->setupUi(this);
}

DiamonIssueRetBro::~DiamonIssueRetBro()
{
    delete ui;
}
