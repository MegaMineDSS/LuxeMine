#include "jewelrymenu.h"
#include "DatabaseUtils.h"
#include <QDebug>

JewelryMenu::JewelryMenu(QObject *parent) : QObject(parent)
{
    menu = new QMenu();
    populateMenu();
}

void JewelryMenu::populateMenu()
{
    QList<QVariantList> menuItems = DatabaseUtils::fetchJewelryMenuItems();

    QMap<int, QMenu*> menuMap;
    menuMap[-1] = menu; // Root menu for top-level categories

    // First pass: Create all top-level categories
    for (const QVariantList &item : menuItems) {
        int id = item[0].toInt();
        int parentId = item[1].isNull() ? -1 : item[1].toInt();
        QString name = item[2].toString();

        if (parentId == -1) {
            QMenu *subMenu = menu->addMenu(name);
            menuMap[id] = subMenu;
        }
    }

    // Second pass: Add sub-items to their parent menus
    for (const QVariantList &item : menuItems) {
        int id = item[0].toInt();
        int parentId = item[1].isNull() ? -1 : item[1].toInt();
        QString name = item[2].toString();
        QString displayText = item[3].toString();

        if (parentId != -1) {
            QMenu *parentMenu = menuMap.value(parentId, nullptr);
            if (parentMenu) {
                QAction *action = parentMenu->addAction(name);
                action->setData(displayText); // Store display text
                connect(action, &QAction::triggered, this, [this, displayText]() {
                    emit itemSelected(displayText);
                });
            } else {
                qDebug() << "Warning: Parent menu not found for item ID" << id;
            }
        }
    }
}

QMenu* JewelryMenu::getMenu() const
{
    return menu;
}
