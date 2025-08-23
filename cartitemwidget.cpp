#include "cartitemwidget.h"
#include <QString>

CartItemWidget::CartItemWidget(int imageId,
                               const QString &goldType,
                               int itemCount,
                               const QPixmap &image,
                               QWidget *parent)
    : QWidget(parent),
    imageId(imageId),
    goldType(goldType),
    itemCount(itemCount),
    image(image)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    // Image
    imageLabel = new QLabel(this);
    imageLabel->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(imageLabel);

    // Details
    detailsLabel = new QLabel(this);
    rebuildDetails();
    layout->addWidget(detailsLabel, /*stretch*/ 1);

    // Quantity controls
    decrementButton = new QPushButton("-", this);
    incrementButton = new QPushButton("+", this);
    quantityLabel   = new QLabel(QString::number(itemCount), this);
    quantityLabel->setMinimumWidth(30);
    quantityLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(decrementButton);
    layout->addWidget(quantityLabel);
    layout->addWidget(incrementButton);

    // Remove button
    removeButton = new QPushButton("Remove", this);
    layout->addWidget(removeButton);

    setLayout(layout);

    connect(incrementButton, &QPushButton::clicked, this, &CartItemWidget::onIncrementClicked);
    connect(decrementButton, &QPushButton::clicked, this, &CartItemWidget::onDecrementClicked);
    connect(removeButton,   &QPushButton::clicked, this, &CartItemWidget::onRemoveClicked);
}

void CartItemWidget::rebuildDetails()
{
    if (!detailsLabel) return;
    detailsLabel->setText(
        QString("Image: %1\nGold: %2").arg(imageId).arg(goldType)
        );
}

void CartItemWidget::setQuantity(int qty)
{
    if (qty < 1) qty = 1;
    itemCount = qty;
    if (quantityLabel) {
        quantityLabel->setText(QString::number(itemCount));
    }
    // IMPORTANT: no signal emitted here (prevents loops)
}

void CartItemWidget::onIncrementClicked()
{
    // Update local UI first
    setQuantity(itemCount + 1);
    // Then notify parent (User)
    emit quantityChanged(imageId, goldType, itemCount);
}

void CartItemWidget::onDecrementClicked()
{
    if (itemCount <= 1) return;  // don’t go below 1
    setQuantity(itemCount - 1);
    emit quantityChanged(imageId, goldType, itemCount);
}

void CartItemWidget::onRemoveClicked()
{
    emit removeRequested(imageId, goldType);
}
