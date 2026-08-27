#pragma once

#include <QWidget>

class QPushButton;
class QLabel;

// Mute button + volume percentage label. Lives inside TransportBar.
class VolumeControl : public QWidget {
    Q_OBJECT

public:
    explicit VolumeControl(QWidget *parent = nullptr);

public slots:
    void setVolume(int percent);
    void setMuted(bool muted);

signals:
    void muteToggleClicked();

private:
    void updateMuteIcon();

    QPushButton *muteButton_ = nullptr;
    QLabel *volumeLabel_ = nullptr;
    int volume_ = 100;
    bool muted_ = false;
};
