#include "ui/SpeedControl.h"
#include "playback/SpeedController.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStandardItemModel>

SpeedControl::SpeedControl(QWidget *parent) : QWidget(parent) {
    magnitudeCombo_ = new QComboBox(this);
    for (double magnitude : SpeedController::kMagnitudeSteps) {
        magnitudeCombo_->addItem(QStringLiteral("%1x").arg(magnitude, 0, 'g', 3), magnitude);
    }
    magnitudeCombo_->setCurrentIndex(SpeedController::kMagnitudeSteps.indexOf(1.0));

    directionButton_ = new QPushButton(QStringLiteral("▶"), this);
    directionButton_->setToolTip(QStringLiteral("正向播放"));
    directionButton_->setFixedWidth(28);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(directionButton_);
    layout->addWidget(magnitudeCombo_);

    connect(magnitudeCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit magnitudeSelected(magnitudeCombo_->itemData(index).toDouble());
    });
    connect(directionButton_, &QPushButton::clicked, this, &SpeedControl::directionToggleClicked);
}

void SpeedControl::setState(double magnitude, bool reverse) {
    // Magnitudes above the reverse cap aren't selectable while reversed --
    // greyed out rather than removed, so the option reappears immediately
    // on switching back to forward.
    if (auto *model = qobject_cast<QStandardItemModel *>(magnitudeCombo_->model())) {
        for (int i = 0; i < magnitudeCombo_->count(); ++i) {
            const double m = magnitudeCombo_->itemData(i).toDouble();
            model->item(i)->setEnabled(!reverse || m <= SpeedController::kReverseMaxMagnitude);
        }
    }

    const int index = SpeedController::kMagnitudeSteps.indexOf(magnitude);
    if (index >= 0) {
        const bool wasBlocked = magnitudeCombo_->blockSignals(true);
        magnitudeCombo_->setCurrentIndex(index);
        magnitudeCombo_->blockSignals(wasBlocked);
    }
    directionButton_->setText(reverse ? QStringLiteral("◀") : QStringLiteral("▶"));
    if (!reverse) {
        directionButton_->setToolTip(QStringLiteral("正向播放"));
    } else {
        directionButton_->setToolTip(QStringLiteral("倒轉播放"));
    }
}

void SpeedControl::showFallbackHint() {
    directionButton_->setToolTip(QStringLiteral("倒轉播放(模擬倒播:無聲音,較不流暢)"));
}
