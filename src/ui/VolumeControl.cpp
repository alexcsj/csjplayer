#include "ui/VolumeControl.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

VolumeControl::VolumeControl(QWidget *parent) : QWidget(parent) {
    muteButton_ = new QPushButton(QStringLiteral("🔊"), this);
    muteButton_->setFixedWidth(32);
    volumeLabel_ = new QLabel(QStringLiteral("100%"), this);
    volumeLabel_->setFixedWidth(36);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(muteButton_);
    layout->addWidget(volumeLabel_);

    connect(muteButton_, &QPushButton::clicked, this, &VolumeControl::muteToggleClicked);
}

void VolumeControl::setVolume(int percent) {
    volume_ = percent;
    volumeLabel_->setText(QStringLiteral("%1%").arg(percent));
    updateMuteIcon();
}

void VolumeControl::setMuted(bool muted) {
    muted_ = muted;
    updateMuteIcon();
}

void VolumeControl::updateMuteIcon() {
    // 0% reads visually the same as muted, even if the "mute" property
    // itself is off (e.g. the user just wheeled the volume all the way down).
    muteButton_->setText(muted_ || volume_ <= 0 ? QStringLiteral("🔇") : QStringLiteral("🔊"));
}
