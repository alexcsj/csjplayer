#pragma once

#include <QDialog>

class QCheckBox;
class QSpinBox;

// Audio/video sync offset, in milliseconds (mpv's "audio-delay" is in
// seconds; this dialog works in ms since that's the unit users think in for
// small lip-sync corrections). Positive = audio delayed relative to video,
// negative = audio advanced. Opened via the right-click menu's
// "調整音訊/視訊同步"; the current live value (MpvController::audioDelayMs())
// seeds the spin box, and OK applies it immediately via setAudioDelayMs().
//
// The "同時寫入檔案" checkbox additionally bakes the offset permanently into
// a new copy of the file (via AudioSyncMuxer/ffmpeg) so other players pick
// it up too -- the live mpv setting alone only lasts for this session.
class AudioSyncDialog : public QDialog {
    Q_OBJECT

public:
    explicit AudioSyncDialog(int currentMs, QWidget *parent = nullptr);

    int valueMs() const;
    bool writeToFile() const;

private:
    QSpinBox *delaySpin_ = nullptr;
    QCheckBox *writeToFileCheck_ = nullptr;
};
