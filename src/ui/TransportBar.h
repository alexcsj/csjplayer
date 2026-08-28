#pragma once

#include <QWidget>

#include <optional>

class QPushButton;
class QLabel;
class SeekBar;
class VolumeControl;
class SpeedControl;

// Play/pause, prev/next, seek bar, time labels, volume/mute, speed/
// direction, and the playlist-panel toggle.
class TransportBar : public QWidget {
    Q_OBJECT

public:
    explicit TransportBar(QWidget *parent = nullptr);

public slots:
    void setPaused(bool paused);
    void setPosition(double seconds);
    void setDuration(double seconds);
    void setPlaylistNavigationEnabled(bool enabled);
    void setVolume(int percent);
    void setMuted(bool muted);
    void setSpeedState(double magnitude, bool reverse);
    void showReverseFallbackHint();
    void setLoopMarkers(std::optional<double> aSeconds, std::optional<double> bSeconds);

signals:
    void playPauseClicked();
    void seekRequested(double seconds);
    void previousClicked();
    void nextClicked();
    void playlistToggleClicked();
    void openFilesClicked();
    void muteToggleClicked();
    void speedMagnitudeSelected(double magnitude);
    void speedDirectionToggleClicked();

private:
    static QString formatTime(double seconds);

    QPushButton *playPauseButton_ = nullptr;
    QPushButton *prevButton_ = nullptr;
    QPushButton *nextButton_ = nullptr;
    QPushButton *openFilesButton_ = nullptr;
    QPushButton *playlistToggleButton_ = nullptr;
    SeekBar *seekBar_ = nullptr;
    VolumeControl *volumeControl_ = nullptr;
    SpeedControl *speedControl_ = nullptr;
    QLabel *currentTimeLabel_ = nullptr;
    QLabel *durationLabel_ = nullptr;
};
