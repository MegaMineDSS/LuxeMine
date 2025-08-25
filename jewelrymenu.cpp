#include <QDebug>

#include "jewelrymenu.h"
#include "databaseutils.h"

JewelryMenu::JewelryMenu(QObject *parent) : QObject(parent)
{
    // Give QMenu a parent if possible, otherwise delete manually in destructor
    menu = new QMenu(static_cast<QWidget*>(parent));

    populateMenu();

    menu->setStyleSheet(R"(
        QMenu {
            background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,
                                              stop:0 #E0E0E0, stop:1 #F2F2F2);
            color: #2C2C2C;
            border: none;
            border-radius: 6px;
            padding: 3px 6px;
            font-size: 14px;
            font-weight: bold;
            text-align: center;
        }
        QMenu::item {
            background-color: transparent;
            padding: 8px 20px;
            margin: 2px 0;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #D0D0D0;
            color: #1A1A1A;
        }
        QMenu::separator {
            height: 1px;
            background: #B0B0B0;
            margin: 6px 10px;
        }
    )");
}

JewelryMenu::~JewelryMenu()
{
    // Only needed if no parent given in constructor
    if (menu && !menu->parent()) {
        delete menu;
        menu = nullptr;
    }
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

