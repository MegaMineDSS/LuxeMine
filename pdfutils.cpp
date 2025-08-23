#include <QPdfWriter>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QPainterPath>
#include <QDesktopServices>
#include <QUrl>

#include "PdfUtils.h"
#include "cartitemwidget.h"


PdfUtils::PdfUtils() {}

bool PdfUtils::generateCartPdf(const QString &pdfPath, const QString &userId, const QString &userName,
                               const QString &companyName, const QString &mobileNo,
                               const QList<SelectionData> &selections, QWidget *cartItemsContainer,
                               QTableWidget *diamondTable, QTableWidget *stoneTable, const QString &goldWeightText)
{
    QPdfWriter writer(pdfPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(5, 5, 5, 5)); // 5mm margins
    writer.setResolution(72);

    QPainter painter(&writer);
    painter.setRenderHint(QPainter::Antialiasing);

    QSizeF contentSize = writer.pageLayout().paintRectPixels(writer.resolution()).size();
    int pageWidth = contentSize.width();
    // int pageHeight = contentSize.height();

    // ========== 1. COVER PAGE ==========
    painter.save();
    painter.setFont(QFont("Georgia", 40));
    painter.setPen(QColor("#d4af37"));
    QString title1 = "Exquisite";
    int yPos = 20 * 72 / 25.4; // 20mm from top
    painter.drawText(QRect(0, yPos, pageWidth, 100), Qt::AlignCenter, title1);

    painter.setFont(QFont("Georgia", 34));
    QString title2 = "Jewelry Collection";
    yPos += 50;
    painter.drawText(QRect(0, yPos, pageWidth, 100), Qt::AlignCenter, title2);

    painter.setFont(QFont("Georgia", 20));
    painter.setPen(Qt::black);
    yPos += 80;
    painter.drawText(QRect(0, yPos, pageWidth, 100), Qt::AlignCenter, QString("Prepared for: %1").arg(userName));

    painter.setFont(QFont("Georgia", 18));
    yPos += 40;
    painter.drawText(QRect(0, yPos, pageWidth, 100), Qt::AlignCenter, QString("Date: %1").arg(QDate::currentDate().toString("MMMM d, yyyy")));

    painter.setFont(QFont("Georgia", 16));
    yPos += 80;
    painter.drawText(QRect(0, yPos, pageWidth, 100), Qt::AlignCenter, companyName);

    painter.setFont(QFont("Georgia", 14));
    painter.setPen(QColor("#555"));
    yPos += 40;
    painter.drawText(QRect(0, yPos, pageWidth, 100), Qt::AlignCenter, QString("Contact: %1 | %2").arg(mobileNo, "contact@megamine.com"));
    painter.restore();

    // ========== 2. IMAGE PAGES ==========
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(cartItemsContainer->layout());
    if (!layout) {
        qDebug() << "Error: cartItemsContainer layout is not a QVBoxLayout";
        return false;
    }

    int itemsPerPage = 3;
    for (int i = 0; i < layout->count(); i += itemsPerPage) {
        writer.newPage();
        painter.save();

        int blockWidth = static_cast<int>(pageWidth * 0.9);
        int blockX = (pageWidth - blockWidth) / 2;
        int blockY = 20;

        for (int j = 0; j < itemsPerPage && (i + j) < layout->count(); ++j) {
            QLayoutItem *layoutItem = layout->itemAt(i + j);
            if (!layoutItem) continue;

            CartItemWidget *itemWidget = qobject_cast<CartItemWidget*>(layoutItem->widget());
            if (!itemWidget) continue;

            // Shadow effect
            QRect blockRect(blockX, blockY, blockWidth, 240);
            QRect shadowRect = blockRect.adjusted(5, 5, 5, 5);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 80));
            painter.drawRoundedRect(shadowRect, 10, 10);

            // White block
            painter.setPen(QPen(QColor("#ccc"), 1));
            painter.setBrush(Qt::white);
            painter.drawRoundedRect(blockRect, 10, 10);

            // Image
            QPixmap pixmap = itemWidget->getImage();
            if (!pixmap.isNull()) {
                QRect imgRect(blockX + 10, blockY + 10, 250, 220);
                QPixmap scaledPixmap = pixmap.scaled(imgRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

                QPainterPath clipPath;
                clipPath.addRoundedRect(imgRect, 10, 10);
                painter.save();
                painter.setClipPath(clipPath);
                painter.drawPixmap(imgRect, scaledPixmap);
                painter.restore();
            }

            // Text
            painter.setFont(QFont("Georgia", 14));
            painter.setPen(QColor("#333333"));
            QRect textRect(blockX + 285, blockY, blockWidth - 285, 240);

            // Fetch diamond details from selection
            QString diamondText = "None";
            if (i + j < selections.size()) {
                QJsonDocument doc = QJsonDocument::fromJson(selections[i + j].diamondJson.toUtf8());
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    QString shape = obj["shape"].toString();
                    double weight = obj["weight"].toDouble();
                    if (!shape.isEmpty() && weight > 0) {
                        diamondText = QString("%1 ct, %2").arg(weight, 0, 'f', 2).arg(shape);
                    }
                }
            }

            QString itemText = QString(
                                   "Jewelry Item #%1\n"
                                   "Type: %2\n"
                                   "Diamond: %3\n"
                                   "Gold: %4, %5g\n"
                                   "Description: Elegant design with fine detailing."
                                   ).arg(i + j + 1)
                                   .arg(itemWidget->getGoldType())
                                   .arg(diamondText)
                                   .arg(itemWidget->getGoldType())
                                   .arg(selections[i + j].itemCount * 10.0, 0, 'f', 1); // Assuming weight per item

            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, itemText);

            blockY += 260;
        }

        painter.restore();
    }

    // ========== 3. SUMMARY PAGE ==========
    writer.newPage();
    painter.save();

    // Title
    painter.setFont(QFont("Georgia", 24));
    painter.setPen(QColor("#d4af37"));
    painter.drawText(QRect(0, 20, pageWidth, 100), Qt::AlignCenter, "Order Summary");

    // Gold Details Table
    painter.setFont(QFont("Georgia", 16));
    painter.setPen(QColor("#333"));
    int tableY = 100;
    painter.drawText(QRect(0, tableY, pageWidth, 50), Qt::AlignLeft, "Gold Details");

    tableY += 30;
    int tableX = 20;
    int colWidth = pageWidth / 2 - 20;
    int rowHeight = 40;

    // Header
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#d4af37"));
    painter.drawRect(tableX, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
    painter.setFont(QFont("Georgia", 12));
    painter.setPen(Qt::white);
    painter.drawText(QRect(tableX + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Gold KT");
    painter.drawText(QRect(tableX + colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Total Weight (grams)");
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(tableX, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);

    // Gold Rows (parsed from goldWeightText or selections)
    QMap<QString, double> goldWeights;
    for (const auto &selection : selections) {
        QString goldType = selection.goldType;
        double weight = selection.itemCount * 10.0; // Assuming weight per item
        goldWeights[goldType] += weight;
    }

    tableY += rowHeight;
    painter.setPen(QColor("#333"));
    for (auto it = goldWeights.constBegin(); it != goldWeights.constEnd(); ++it) {
        painter.drawText(QRect(tableX + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, it.key());
        painter.drawText(QRect(tableX + colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value(), 'f', 1));
        painter.drawRect(tableX, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
        tableY += rowHeight;
    }

    // Diamond Details Table
    tableY += 40;
    painter.setFont(QFont("Georgia", 16));
    painter.drawText(QRect(0, tableY, pageWidth, 50), Qt::AlignLeft, "Diamond Details");

    tableY += 30;
    colWidth = pageWidth / 4 - 10;
    // Header
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#d4af37"));
    painter.drawRect(tableX, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 2 * colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 3 * colWidth, tableY, colWidth, rowHeight);
    painter.setPen(Qt::white);
    painter.drawText(QRect(tableX + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Carat");
    painter.drawText(QRect(tableX + colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Clarity");
    painter.drawText(QRect(tableX + 2 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Color");
    painter.drawText(QRect(tableX + 3 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Count");
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(tableX, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 2 * colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 3 * colWidth, tableY, colWidth, rowHeight);

    // Diamond Rows
    for (int row = 0; row < diamondTable->rowCount(); ++row) {
        tableY += rowHeight;
        painter.setPen(QColor("#333"));
        QString carat = diamondTable->item(row, 3) ? diamondTable->item(row, 3)->text() : ""; // Weight
        QString clarity = diamondTable->item(row, 0) ? diamondTable->item(row, 0)->text() : ""; // Shape/Type
        QString color = diamondTable->item(row, 1) ? diamondTable->item(row, 1)->text() : ""; // SizeMM
        QString count = diamondTable->item(row, 2) ? diamondTable->item(row, 2)->text() : ""; // Quantity
        painter.drawText(QRect(tableX + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, carat);
        painter.drawText(QRect(tableX + colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, clarity);
        painter.drawText(QRect(tableX + 2 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, color);
        painter.drawText(QRect(tableX + 3 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, count);
        painter.drawRect(tableX, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + 2 * colWidth, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + 3 * colWidth, tableY, colWidth, rowHeight);
    }

    // Stone Details Table
    tableY += 40;
    painter.setFont(QFont("Georgia", 16));
    painter.drawText(QRect(0, tableY, pageWidth, 50), Qt::AlignLeft, "Stone Details");

    tableY += 30;
    // Header
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#d4af37"));
    painter.drawRect(tableX, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 2 * colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 3 * colWidth, tableY, colWidth, rowHeight);
    painter.setPen(Qt::white);
    painter.drawText(QRect(tableX + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Carat");
    painter.drawText(QRect(tableX + colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Type");
    painter.drawText(QRect(tableX + 2 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Size");
    painter.drawText(QRect(tableX + 3 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, "Count");
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(tableX, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 2 * colWidth, tableY, colWidth, rowHeight);
    painter.drawRect(tableX + 3 * colWidth, tableY, colWidth, rowHeight);

    // Stone Rows
    for (int row = 0; row < stoneTable->rowCount(); ++row) {
        tableY += rowHeight;
        painter.setPen(QColor("#333"));
        QString carat = stoneTable->item(row, 3) ? stoneTable->item(row, 3)->text() : "";
        QString type = stoneTable->item(row, 0) ? stoneTable->item(row, 0)->text() : "";
        QString size = stoneTable->item(row, 1) ? stoneTable->item(row, 1)->text() : "";
        QString count = stoneTable->item(row, 2) ? stoneTable->item(row, 2)->text() : "";
        painter.drawText(QRect(tableX + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, carat);
        painter.drawText(QRect(tableX + colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, type);
        painter.drawText(QRect(tableX + 2 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, size);
        painter.drawText(QRect(tableX + 3 * colWidth + 10, tableY, colWidth, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, count);
        painter.drawRect(tableX, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + colWidth, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + 2 * colWidth, tableY, colWidth, rowHeight);
        painter.drawRect(tableX + 3 * colWidth, tableY, colWidth, rowHeight);
    }

    // Footer
    painter.setFont(QFont("Georgia", 12));
    painter.setPen(QColor("#555"));
    painter.drawText(QRect(0, tableY + 80, pageWidth, 50), Qt::AlignCenter, "Thank you for your trust!");

    painter.restore();
    painter.end();

    QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
    return true;
}

