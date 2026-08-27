#include "ui/TransportBar.h"
#include "ui/SeekBar.h"
#include "ui/SpeedControl.h"
#include "ui/VolumeControl.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

TransportBar::TransportBar(QWidget *parent) : QWidget(parent) {
    prevButton_ = new QPushButton(QStringLiteral("|<"), this);
    playPauseButton_ = new QPushButton(QStringLiteral("Pause"), this);
    nextButton_ = new QPushButton(QStringLiteral(">|"), this);
    seekBar_ = new SeekBar(this);
    currentTimeLabel_ = new QLabel(QStringLiteral("00:00"), this);
    durationLabel_ = new QLabel(QStringLiteral("00:00"), this);
    volumeControl_ = new VolumeControl(this);
    speedControl_ = new SpeedControl(this);
    openFilesButton_ = new QPushButton(QStringLiteral("⏏"), this); // triangle-over-bar "open" glyph
    openFilesButton_->setToolTip(QStringLiteral("開啟檔案"));
    playlistToggleButton_ = new QPushButton(QStringLiteral("清單"), this);

    // Nothing to skip to until the playlist has at least one entry.
    prevButton_->setEnabled(false);
    nextButton_->setEnabled(false);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(prevButton_);
    layout->addWidget(playPauseButton_);
    layout->addWidget(nextButton_);
    layout->addWidget(currentTimeLabel_);
    layout->addWidget(seekBar_, /*stretch=*/1);
    layout->addWidget(durationLabel_);
    layout->addWidget(speedControl_);
    layout->addWidget(volumeControl_);
    layout->addWidget(openFilesButton_);
    layout->addWidget(playlistToggleButton_);

    connect(playPauseButton_, &QPushButton::clicked, this, &TransportBar::playPauseClicked);
    connect(seekBar_, &SeekBar::seekRequested, this, &TransportBar::seekRequested);
    connect(prevButton_, &QPushButton::clicked, this, &TransportBar::previousClicked);
    connect(nextButton_, &QPushButton::clicked, this, &TransportBar::nextClicked);
    connect(openFilesButton_, &QPushButton::clicked, this, &TransportBar::openFilesClicked);
    connect(playlistToggleButton_, &QPushButton::clicked, this, &TransportBar::playlistToggleClicked);
    connect(volumeControl_, &VolumeControl::muteToggleClicked, this, &TransportBar::muteToggleClicked);
    connect(speedControl_, &SpeedControl::magnitudeSelected, this, &TransportBar::speedMagnitudeSelected);
    connect(speedControl_, &SpeedControl::directionToggleClicked, this, &TransportBar::speedDirectionToggleClicked);
}

void TransportBar::setVolume(int percent) {
    volumeControl_->setVolume(percent);
}

void TransportBar::setMuted(bool muted) {
    volumeControl_->setMuted(muted);
}

void TransportBar::setSpeedState(double magnitude, bool reverse) {
    speedControl_->setState(magnitude, reverse);
}

void TransportBar::showReverseFallbackHint() {
    speedControl_->showFallbackHint();
}

void TransportBar::setPlaylistNavigationEnabled(bool enabled) {
    prevButton_->setEnabled(enabled);
    nextButton_->setEnabled(enabled);
}

void TransportBar::setPaused(bool paused) {
    playPauseButton_->setText(paused ? QStringLiteral("Play") : QStringLiteral("Pause"));
}

void TransportBar::setPosition(double seconds) {
    seekBar_->setPositionSeconds(seconds);
    currentTimeLabel_->setText(formatTime(seconds));
}

void TransportBar::setDuration(double seconds) {
    seekBar_->setDurationSeconds(seconds);
    durationLabel_->setText(formatTime(seconds));
}

QString TransportBar::formatTime(double seconds) {
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    }
    return QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}
