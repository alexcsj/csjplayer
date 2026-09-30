#pragma once

#include "playback/PlaylistController.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
struct WindowSizePresets;

// What the app starts up looking/sounding like. Persisted via QSettings by
// PlayerWindow, editable through StartupDefaultsDialog (right-click menu's
// "預設畫面及音量").
struct StartupDefaults {
    // Preset1..Preset5 mirror whatever Alt+1..Alt+5 are currently bound to
    // (WindowSizePresets) -- looked up at startup, not frozen here, so
    // editing a preset's size later also updates what this uses.
    enum class SizeMode { Preset1, Preset2, Preset3, Preset4, Preset5, Custom };

    SizeMode sizeMode = SizeMode::Preset1;
    int customWidth = 960;
    int customHeight = 540;
    int volume = 100;
    bool playlistPanelVisible = false;
    PlaylistController::RepeatMode repeatMode = PlaylistController::RepeatMode::NoRepeat;
};

class StartupDefaultsDialog : public QDialog {
    Q_OBJECT

public:
    // sizePresets supplies the Alt+1..Alt+5 dimensions shown next to each
    // size-mode option, so the dialog doesn't need PlayerWindow to resolve
    // them upfront.
    explicit StartupDefaultsDialog(const StartupDefaults &current, const WindowSizePresets &sizePresets,
                                    QWidget *parent = nullptr);

    StartupDefaults values() const;

private:
    void updateCustomSizeEnabled();

    QComboBox *sizeModeCombo_ = nullptr;
    QSpinBox *customWidthSpin_ = nullptr;
    QSpinBox *customHeightSpin_ = nullptr;
    QSpinBox *volumeSpin_ = nullptr;
    QCheckBox *playlistPanelCheck_ = nullptr;
    QComboBox *repeatModeCombo_ = nullptr;
};
