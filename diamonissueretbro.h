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

private:
    Ui::DiamonIssueRetBro *ui;
};

#endif // DIAMONISSUERETBRO_H
