#pragma once

#include <QDialog>

class QSpinBox;

// Window size (width x height) for each of the Alt+1..Alt+5 shortcuts.
// Persisted via QSettings by PlayerWindow, editable through
// WindowSizePresetsDialog (right-click menu's "調整視窗大小快捷鍵").
struct WindowSizePreset {
    int width = 0;
    int height = 0;
};

struct WindowSizePresets {
    // presets[0] -> Alt+1, presets[1] -> Alt+2, ... presets[4] -> Alt+5.
    WindowSizePreset presets[5] = {
        {960, 540}, {1920, 1080}, {3840, 2160}, {540, 960}, {1080, 1920},
    };

    // Thickness (px) of the invisible edge-resize hit-zone frame around the
    // video widget (MpvGLWidget::setResizeMarginPx()).
    int resizeMarginPx = 5;
};

class WindowSizePresetsDialog : public QDialog {
    Q_OBJECT

public:
    explicit WindowSizePresetsDialog(const WindowSizePresets &current, QWidget *parent = nullptr);

    WindowSizePresets values() const;

private:
    QSpinBox *widthSpins_[5] = {};
    QSpinBox *heightSpins_[5] = {};
    QSpinBox *resizeMarginSpin_ = nullptr;
};
