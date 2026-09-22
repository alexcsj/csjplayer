#pragma once

#include <QDialog>

class QSpinBox;

// Audio/video sync offset, in milliseconds (mpv's "audio-delay" is in
// seconds; this dialog works in ms since that's the unit users think in for
// small lip-sync corrections). Positive = audio delayed relative to video,
// negative = audio advanced. Opened via the right-click menu's
// "調整音訊/視訊同步"; the current live value (MpvController::audioDelayMs())
// seeds the spin box, and OK applies it immediately via setAudioDelayMs().
class AudioSyncDialog : public QDialog {
    Q_OBJECT

public:
    explicit AudioSyncDialog(int currentMs, QWidget *parent = nullptr);

    int valueMs() const;

private:
    QSpinBox *delaySpin_ = nullptr;
};
