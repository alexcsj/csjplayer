#include "ui/SeekBar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPolygon>
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
    // Some containers/formats refine their reported duration slightly as
    // mpv demuxes further into the file, re-firing this with near-identical
    // values. Re-applying setRange() every time visibly jittered the handle
    // (and the loop markers, which share the same value/range mapping), so
    // skip it unless the rounded duration actually changed.
    const int rounded = qRound(seconds);
    if (rounded != maximum()) {
        setRange(0, rounded);
    }
}

void SeekBar::setPositionSeconds(double seconds) {
    if (userIsDragging_) {
        return;
    }
    // Rounding (not truncating) reduces how often minor AV-sync jitter in
    // mpv's reported time-pos flips the displayed second back and forth
    // across a whole-second boundary.
    const bool wasBlocked = blockSignals(true);
    setValue(qRound(seconds));
    blockSignals(wasBlocked);
}

void SeekBar::setLoopMarkers(std::optional<double> aSeconds, std::optional<double> bSeconds) {
    loopAMarker_ = aSeconds;
    loopBMarker_ = bSeconds;
    update();
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

void SeekBar::paintEvent(QPaintEvent *event) {
    QSlider::paintEvent(event);

    if (!loopAMarker_ && !loopBMarker_) {
        return;
    }
    if (maximum() <= minimum()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    auto drawMarker = [&](double seconds, const QColor &color) {
        const double ratio = (seconds - minimum()) / static_cast<double>(maximum() - minimum());
        const int x = static_cast<int>(ratio * width());
        painter.setBrush(color);
        const QPolygon triangle({QPoint(x - 4, 0), QPoint(x + 4, 0), QPoint(x, 7)});
        painter.drawPolygon(triangle);
    };

    if (loopAMarker_) {
        drawMarker(*loopAMarker_, QColor(76, 175, 80));
    }
    if (loopBMarker_) {
        drawMarker(*loopBMarker_, QColor(244, 67, 54));
    }
}
