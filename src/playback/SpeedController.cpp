#include "playback/SpeedController.h"
#include "mpv/MpvController.h"

#include <QTimer>

#include <algorithm>

namespace {
constexpr int kReverseHealthGraceMs = 1000;
constexpr double kReverseHealthMinDelta = 0.05; // seconds; below this counts as "didn't move"
constexpr int kSimTickMs = 100;
} // namespace

const QVector<double> SpeedController::kMagnitudeSteps = {0.1, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0};

SpeedController::SpeedController(MpvController *mpvController, QObject *parent)
    : QObject(parent), mpvController_(mpvController) {
    connect(mpvController_, &MpvController::positionChanged, this, &SpeedController::onPositionChanged);
}

void SpeedController::setMagnitude(double magnitude) {
    magnitude_ = isReverse() ? std::min(magnitude, kReverseMaxMagnitude) : magnitude;
    switch (mode_) {
    case Mode::Forward:
    case Mode::ReverseNative:
        mpvController_->setSpeed(magnitude_);
        break;
    case Mode::ReverseSimulated:
        // Timer interval is fixed; simulatedReverseTick() reads magnitude_
        // directly each tick, so nothing else to do here.
        break;
    }
    emit stateChanged();
}

void SpeedController::toggleDirection() {
    if (mode_ == Mode::Forward) {
        magnitude_ = std::min(magnitude_, kReverseMaxMagnitude);
        enterReverseNative();
    } else {
        enterForward();
    }
}

void SpeedController::resetToForwardNormal() {
    if (simTimer_) {
        simTimer_->stop();
    }
    verifyArmed_ = false;
    mode_ = Mode::Forward;
    magnitude_ = 1.0;
    mpvController_->setPlayDirectionBackward(false);
    mpvController_->setSpeed(1.0);
    emit stateChanged();
}

void SpeedController::enterForward() {
    if (mode_ == Mode::ReverseSimulated && simTimer_) {
        simTimer_->stop();
    }
    verifyArmed_ = false;
    mode_ = Mode::Forward;
    mpvController_->setPlayDirectionBackward(false);
    mpvController_->setSpeed(magnitude_);
    mpvController_->setPaused(false);
    emit stateChanged();
}

void SpeedController::enterReverseNative() {
    mode_ = Mode::ReverseNative;
    mpvController_->setPlayDirectionBackward(true);
    mpvController_->setSpeed(magnitude_);
    mpvController_->setPaused(false);

    verifyArmed_ = true;
    verifyEntryPosition_ = -1.0;
    QTimer::singleShot(kReverseHealthGraceMs, this, &SpeedController::checkReverseHealth);

    emit stateChanged();
}

void SpeedController::enterReverseSimulated() {
    mode_ = Mode::ReverseSimulated;
    verifyArmed_ = false;

    // Fully disengage native reverse first so the two mechanisms don't
    // fight; simulated reverse drives position purely via relative seeks
    // issued on a timer while nominally paused (mpv still updates the
    // displayed frame on seek even when paused). This is video-only: mpv
    // has no backward audio playback primitive.
    mpvController_->setPlayDirectionBackward(false);
    mpvController_->setSpeed(1.0);
    mpvController_->setPaused(true);

    if (!simTimer_) {
        simTimer_ = new QTimer(this);
        connect(simTimer_, &QTimer::timeout, this, &SpeedController::simulatedReverseTick);
    }
    simTimer_->start(kSimTickMs);

    emit stateChanged();
}

void SpeedController::onPositionChanged(double seconds) {
    if (verifyArmed_ && verifyEntryPosition_ < 0.0) {
        verifyEntryPosition_ = seconds;
    }
    lastPosition_ = seconds;
}

void SpeedController::checkReverseHealth() {
    if (mode_ != Mode::ReverseNative || !verifyArmed_) {
        return;
    }
    verifyArmed_ = false;

    const bool movedBackward =
        verifyEntryPosition_ >= 0.0 && (verifyEntryPosition_ - lastPosition_) >= kReverseHealthMinDelta;
    if (!movedBackward) {
        emit reverseFallbackEngaged();
        enterReverseSimulated();
    }
}

void SpeedController::simulatedReverseTick() {
    const double step = magnitude_ * (kSimTickMs / 1000.0);
    if (lastPosition_ >= 0.0 && lastPosition_ - step <= 0.0) {
        mpvController_->seekAbsolute(0);
        simTimer_->stop();
        return;
    }
    mpvController_->seekRelative(-step);
}
