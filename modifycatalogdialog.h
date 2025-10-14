#ifndef MODIFYCATALOGDIALOG_H
#define MODIFYCATALOGDIALOG_H

#include <QDialog>
#include <QJsonArray>

namespace Ui {
class ModifyCatalogDialog;
}

class ModifyCatalogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModifyCatalogDialog(const QString &designNo, QWidget *parent = nullptr);
    ~ModifyCatalogDialog() override;


private:
    QString m_designNo;          // store the design number being modified
    QString m_loadedImagePath;   // original DB image path
    QString m_newSelectedImage;  // new image path if user browsed

    // UI widgets (since you're creating manually)
    QLabel *imagePreview {nullptr};
    QLineEdit *imagePathEdit {nullptr};
    QPushButton *browseBtn {nullptr};
    QLineEdit *companyEdit {nullptr};
    QLineEdit *imageTypeEdit {nullptr};
    QTableWidget *goldTable {nullptr};
    QTableWidget *diamondTable {nullptr};
    QTableWidget *stoneTable {nullptr};
    QLineEdit *noteEdit {nullptr};

private slots:
    void onBrowseImage();
    void onAddDiamondRow();
    void onRemoveDiamondRow();
    void onAddStoneRow();
    void onRemoveStoneRow();
    void onSaveClicked();
    void onGoldktChanged();

    bool loadData();
    void populateGoldTableFromJson(const QJsonArray &goldArray);
    void populateDiamondTableFromJson(const QJsonArray &diamondArray);
    void populateStoneTableFromJson(const QJsonArray &stoneArray);


private:
    Ui::ModifyCatalogDialog *ui;
};

#endif // MODIFYCATALOGDIALOG_H
