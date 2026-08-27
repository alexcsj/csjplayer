#include "ui/SeekBar.h"

#include <QMouseEvent>
#include <QStyle>
#include <QWheelEvent>

SeekBar::SeekBar(QWidget *parent) : QSlider(Qt::Horizontal, parent) {
    // Never grabs keyboard focus, so arrow-key seek/volume shortcuts (owned
    // globally by PlayerWindow) reach the window instead of being consumed
    // here by QSlider's default single-step key handling.
    setFocusPolicy(Qt::NoFocus);

    connect(this, &QAbstractSlider::sliderPressed, this, [this]() { userIsDragging_ = true; });
    connect(this, &QAbstractSlider::sliderReleased, this, [this]() {
        userIsDragging_ = false;
        emit seekRequested(static_cast<double>(value()));
    });
}

void SeekBar::setDurationSeconds(double seconds) {
    setRange(0, static_cast<int>(seconds));
}

void SeekBar::setPositionSeconds(double seconds) {
    if (userIsDragging_) {
        return;
    }
    const bool wasBlocked = blockSignals(true);
    setValue(static_cast<int>(seconds));
    blockSignals(wasBlocked);
}

void SeekBar::wheelEvent(QWheelEvent *event) {
    // Global wheel gestures (F6 seek / F7 volume, per PlayerWindow's
    // centralized handling in a later milestone) take precedence over
    // QSlider's default "wheel scrubs the value" behavior.
    event->ignore();
}

namespace {
int valueFromX(const SeekBar *bar, int x) {
    return QStyle::sliderValueFromPosition(bar->minimum(), bar->maximum(), x, bar->width());
}
} // namespace

void SeekBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QSlider::mousePressEvent(event);
        return;
    }
    // setSliderDown(true) puts QAbstractSlider itself into the "down" state
    // (emitting sliderPressed(), which our constructor already wires to
    // userIsDragging_) without going through the style's handle hit-test.
    setSliderDown(true);
    setValue(valueFromX(this, static_cast<int>(event->position().x())));
    event->accept();
}

void SeekBar::mouseMoveEvent(QMouseEvent *event) {
    if (!isSliderDown()) {
        QSlider::mouseMoveEvent(event);
        return;
    }
    setValue(valueFromX(this, static_cast<int>(event->position().x())));
    event->accept();
}

void SeekBar::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !isSliderDown()) {
        QSlider::mouseReleaseEvent(event);
        return;
    }
    setSliderDown(false); // emits sliderReleased(), which fires seekRequested()
    event->accept();
}
