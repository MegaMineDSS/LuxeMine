#ifndef JEWELRYMENU_H
#define JEWELRYMENU_H

#include <QMenu>
#include <QObject>

class JewelryMenu : public QObject
{
    Q_OBJECT

public:
    explicit JewelryMenu(QObject *parent = nullptr);
    QMenu* getMenu() const; // Provide access to the menu for connection

signals:
    void itemSelected(const QString &item); // Signal emitted when an item is selected

private:
    void populateMenu(); // Load menu structure from database
    QMenu *menu; // The main jewelry menu
};

#endif // JEWELRYMENU_H
