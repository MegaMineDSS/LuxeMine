#ifndef JEWELRYMENU_H
#define JEWELRYMENU_H

#include <QMenu>
#include <QObject>
#include <QString>

class JewelryMenu : public QObject
{
    Q_OBJECT

public:
    explicit JewelryMenu(QObject *parent = nullptr);
    ~JewelryMenu() override;  // ensure base QObject dtor is virtual

    QMenu* getMenu() const;   // Access the built menu

signals:
    void itemSelected(const QString &item); // Emitted when user selects an item

private:
    void populateMenu();
    QMenu *menu {nullptr};   // initialize to nullptr for safety
};

#endif // JEWELRYMENU_H
