#ifndef ADMIN_H
#define ADMIN_H

#include <QDialog>
#include <QSqlTableModel>
#include <QStringList>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>

namespace Ui {
class Admin;
}

class Admin : public QDialog
{
    Q_OBJECT

public:
    explicit Admin(QWidget *parent = nullptr);
    ~Admin();

private slots:
    // void closeEvent(QCloseEvent *event);
    void on_login_button_clicked();
    void on_cancel_button_clicked();
    void on_show_images_clicked();
    void on_update_price_clicked();
    void on_show_users_clicked();
    void on_logout_clicked();
    void on_add_dia_clicked();
    void on_gold_button_clicked();
    void on_dia_button_clicked();
    void on_round_dia_button_clicked();
    void on_fancy_dia_button_clicked();
    void on_AddRoundDiamond_clicked();
    void on_AddFancyDiamond_clicked();
    void on_RoundDiamind_Price_clicked();
    void on_updateRoundDiamond_price_clicked();
    void on_FancyDiamond_Price_clicked();
    void on_updateFancyDiamond_price_clicked();
    void on_upadateGold_Price_clicked();
    // void on_cbackup_clicked();
    // void on_rbackup_clicked();
    void on_deleteUser_triggered();
    void on_tableWidget_2_cellDoubleClicked(int row, int column); // New slot for PDF clicks
    void on_jewelry_menu_button_clicked();
    void on_add_menu_category_clicked(); // Added for adding top-level categories
    void on_add_menu_item_clicked(); // Added for adding sub-items
    void on_delete_menu_item_clicked(); // Added for deleting items

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Admin *ui;
    QSqlTableModel *roundDiamondModel = nullptr;
    QSqlTableModel *fancyDiamondModel = nullptr;
    QStringList imagePaths;
    int currentIndex = 0;
    bool m_menuVisible = false;

    // QStackedWidget *stackWidget;  // The stacked widget used in admin window
    bool checkLoginCredentials(const QString &username, const QString &password);  // Helper for login
    void populateJewelryMenuTable(); // Populate table with menu items
    void setupJewelryMenuPage(); // New method to create UI elements
    void populateParentCategoryComboBox();

    // Widgets for jewelry menu page
    QLineEdit *categoryNameLineEdit = nullptr;
    QLineEdit *itemNameLineEdit = nullptr;
    QLineEdit *displayTextLineEdit = nullptr;
    QComboBox *parentCategoryComboBox = nullptr;
    QTableWidget *jewelryMenuTable = nullptr;
    QPushButton *addCategoryButton = nullptr;
    QPushButton *addItemButton = nullptr;
    QPushButton *deleteItemButton = nullptr;

};

#endif // ADMIN_H
