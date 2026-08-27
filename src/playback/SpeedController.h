#pragma once

#include <QObject>
#include <QVector>

class MpvController;
class QTimer;

// F8: forward/reverse playback at 0.1x-32x. Magnitude ("speed") and
// direction ("play-dir") are independent mpv concepts, tracked here as one
// combined state since the UI only ever changes one at a time.
//
// Reverse playback has two implementations, tried in order:
//  1. ReverseNative: mpv's own (experimental) play-dir=backward. Smooth
//     when it works, but not guaranteed for every codec/container, and can
//     silently stall.
//  2. ReverseSimulated: a fallback that pauses playback and periodically
//     seeks backward by a step sized to approximate the selected speed.
//     Video-only -- mpv has no backward audio playback primitive, so audio
//     is silent while simulated reverse is active.
//
// A fresh Forward -> reverse transition always retries native first (never
// permanently remembers a past failure, since viability can be
// file-specific); switching magnitude while already reverse does not.
class SpeedController : public QObject {
    Q_OBJECT

public:
    enum class Mode { Forward, ReverseNative, ReverseSimulated };

    static const QVector<double> kMagnitudeSteps; // 0.1, 0.5, 1, 2, 4, 8, 16, 32

    // Reverse (native or simulated) gets flaky/heavy at very high multiples,
    // so it's capped lower than forward's full 32x.
    static constexpr double kReverseMaxMagnitude = 8.0;

    explicit SpeedController(MpvController *mpvController, QObject *parent = nullptr);

    Mode mode() const { return mode_; }
    double magnitude() const { return magnitude_; }
    bool isReverse() const { return mode_ != Mode::Forward; }

public slots:
    // Keeps the current direction, changes only the magnitude (clamped to
    // kReverseMaxMagnitude while reversed).
    void setMagnitude(double magnitude);

    // Flips Forward <-> reverse at the current magnitude (clamped to
    // kReverseMaxMagnitude when entering reverse). Entering reverse always
    // attempts ReverseNative first.
    void toggleDirection();

    // Stops any active reverse timers/probes and returns to plain forward
    // 1x playback. Called on every playlist track switch -- without this,
    // e.g. ReverseSimulated's tick timer kept running and seeking the
    // *newly loaded* file, which looked like the new file starting from a
    // random position instead of 0:00.
    void resetToForwardNormal();

signals:
    // Magnitude and/or mode changed; UI should refresh its display.
    void stateChanged();

    // ReverseNative was attempted and failed verification, so
    // ReverseSimulated just engaged instead -- UI can show a hint.
    void reverseFallbackEngaged();

private slots:
    void onPositionChanged(double seconds);
    void checkReverseHealth();
    void simulatedReverseTick();

private:
    void enterForward();
    void enterReverseNative();
    void enterReverseSimulated();

    MpvController *mpvController_ = nullptr;
    Mode mode_ = Mode::Forward;
    double magnitude_ = 1.0;

    // ReverseNative fallback-detection: captures position at entry, then
    // checks once after a short grace period whether it actually moved
    // backward.
    bool verifyArmed_ = false;
    double verifyEntryPosition_ = -1.0;
    double lastPosition_ = -1.0;

    QTimer *simTimer_ = nullptr;
};
