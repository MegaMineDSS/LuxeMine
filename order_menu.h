#ifndef ORDER_MENU_H
#define ORDER_MENU_H

#include <QMenu>
#include <QObject>


namespace Ui {
class User;
}


class OrderMenu : public QObject
{
    Q_OBJECT

public:
    explicit OrderMenu(QWidget *parent = nullptr);
    ~OrderMenu();

signals:

private:
    void combo_Box_Check_Box_Bill_Details();
};

#endif // ORDER_MENU_H
