#include "cartitemwidget.h"

CartItemWidget::CartItemWidget(int imageId, const QString &goldType, int itemCount, const QPixmap &image, QWidget *parent) :
    QWidget(parent),
    imageId(imageId),
    goldType(goldType),
    itemCount(itemCount),
    image(image)
{

    // Image
    imageLabel = new QLabel(this);
    imageLabel->setPixmap(image.scaled(50, 50, Qt::KeepAspectRatio));
    layout->addWidget(imageLabel);

    // Details (Image ID and Gold Type)
    detailsLabel = new QLabel(QString("Image ID: %1 - %2").arg(imageId).arg(goldType), this);
    layout->addWidget(detailsLabel);

    // Quantity controls
    QHBoxLayout *quantityLayout = new QHBoxLayout;
    decrementButton = new QPushButton("-", this);
    quantityLabel = new QLabel(QString::number(itemCount), this);
    incrementButton = new QPushButton("+", this);

    quantityLabel->setAlignment(Qt::AlignCenter);
    quantityLabel->setFixedWidth(30); // Adjust width to match the design

    quantityLayout->addWidget(decrementButton);
    quantityLayout->addWidget(quantityLabel);
    quantityLayout->addWidget(incrementButton);
    layout->addLayout(quantityLayout);

    // Remove button
    removeButton = new QPushButton("Remove", this);
    layout->addWidget(removeButton);

    // Connect signals
    connect(incrementButton, &QPushButton::clicked, this, &CartItemWidget::onIncrementClicked);
    connect(decrementButton, &QPushButton::clicked, this, &CartItemWidget::onDecrementClicked);
    connect(removeButton, &QPushButton::clicked, this, &CartItemWidget::onRemoveClicked);

    setLayout(layout);
}

void CartItemWidget::onIncrementClicked() {
    itemCount++;
    quantityLabel->setText(QString::number(itemCount));
    emit quantityChanged(imageId, goldType, itemCount);
}

void CartItemWidget::onDecrementClicked() {
    if (itemCount > 1) { // Don't allow quantity to go below 1
        itemCount--;
        quantityLabel->setText(QString::number(itemCount));
        emit quantityChanged(imageId, goldType, itemCount);
    }
}

void CartItemWidget::onRemoveClicked() {
    emit removeRequested(imageId, goldType);
}
