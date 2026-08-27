#pragma once

#include <QWidget>

class QComboBox;
class QPushButton;

// F8 UI: a magnitude dropdown (0.1x-32x) plus a separate direction toggle
// button -- kept as two independent controls since mpv's own "speed" and
// "play-dir" are independent properties (see SpeedController).
class SpeedControl : public QWidget {
    Q_OBJECT

public:
    explicit SpeedControl(QWidget *parent = nullptr);

public slots:
    void setState(double magnitude, bool reverse);
    void showFallbackHint();

signals:
    void magnitudeSelected(double magnitude);
    void directionToggleClicked();

private:
    QComboBox *magnitudeCombo_ = nullptr;
    QPushButton *directionButton_ = nullptr;
};
