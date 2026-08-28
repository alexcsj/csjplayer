#pragma once

#include <QSlider>

#include <optional>

// A QSlider subclass for the playback progress bar. Guards against two
// classic footguns: (1) fighting the user's drag with programmatic position
// updates from mpv, (2) QSlider's default focus/wheel handling silently
// swallowing the arrow-key and wheel shortcuts owned globally by
// PlayerWindow (F6/F7, wired up in a later milestone).
class SeekBar : public QSlider {
    Q_OBJECT

public:
    explicit SeekBar(QWidget *parent = nullptr);

    // Range is in whole seconds; sub-second resolution isn't needed for a
    // draggable UI slider.
    void setDurationSeconds(double seconds);

    // Ignored while the user is actively dragging the handle, so mpv's
    // position updates don't fight the drag.
    void setPositionSeconds(double seconds);

public slots:
    // F10: A-B loop marker positions. nullopt hides that marker (used for
    // points the user hasn't explicitly set -- see MpvController::setLoopA/B).
    void setLoopMarkers(std::optional<double> aSeconds, std::optional<double> bSeconds);

signals:
    // Emitted only on release (or click-to-seek), never during drag -- see
    // class comment.
    void seekRequested(double seconds);

protected:
    void wheelEvent(QWheelEvent *event) override;

    // Left-button press/move/release are fully hand-rolled (not delegated to
    // QSlider's base implementation) so click-to-seek is 100% reliable. The
    // base implementation re-hit-tests the handle rect against the *style's*
    // geometry after we move it, and integer-rounding between the clicked
    // pixel and the snapped handle position can occasionally miss, silently
    // falling back to a small page-step instead of jumping to the click.
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool userIsDragging_ = false;
    std::optional<double> loopAMarker_;
    std::optional<double> loopBMarker_;
};
