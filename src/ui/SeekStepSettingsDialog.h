#pragma once

#include <QDialog>

class QSpinBox;

// Seek step sizes (in seconds) for the plain Left/Right shortcuts and each
// modifier combo. Persisted via QSettings by PlayerWindow, editable through
// SeekStepSettingsDialog (right-click menu's "調整快轉/回轉時間").
struct SeekStepSettings {
    int plainSeconds = 10;
    int zSeconds = 1;
    int xSeconds = 30;
    int cSeconds = 60;
    int ctrlSeconds = 300;
};

class SeekStepSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SeekStepSettingsDialog(const SeekStepSettings &current, QWidget *parent = nullptr);

    SeekStepSettings values() const;

private:
    QSpinBox *plainSpin_ = nullptr;
    QSpinBox *zSpin_ = nullptr;
    QSpinBox *xSpin_ = nullptr;
    QSpinBox *cSpin_ = nullptr;
    QSpinBox *ctrlSpin_ = nullptr;
};
