#include "ui/StartupDefaultsDialog.h"
#include "ui/WindowSizePresetsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QString repeatModeLabel(PlaylistController::RepeatMode mode) {
    switch (mode) {
    case PlaylistController::RepeatMode::RepeatPlaylist:
        return QStringLiteral("清單循環");
    case PlaylistController::RepeatMode::RepeatSingle:
        return QStringLiteral("單集循環");
    case PlaylistController::RepeatMode::PauseAtEnd:
        return QStringLiteral("播完暫停");
    case PlaylistController::RepeatMode::NoRepeat:
    default:
        return QStringLiteral("不循環");
    }
}

} // namespace

StartupDefaultsDialog::StartupDefaultsDialog(const StartupDefaults &current, const WindowSizePresets &sizePresets,
                                              QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("預設畫面及音量"));

    sizeModeCombo_ = new QComboBox(this);
    for (int i = 0; i < 5; ++i) {
        const WindowSizePreset &preset = sizePresets.presets[i];
        sizeModeCombo_->addItem(QStringLiteral("Alt+%1 (%2 x %3)").arg(i + 1).arg(preset.width).arg(preset.height));
    }
    sizeModeCombo_->addItem(QStringLiteral("自訂"));
    sizeModeCombo_->setCurrentIndex(static_cast<int>(current.sizeMode));

    customWidthSpin_ = new QSpinBox(this);
    customWidthSpin_->setRange(64, 7680);
    customWidthSpin_->setValue(current.customWidth);
    customHeightSpin_ = new QSpinBox(this);
    customHeightSpin_->setRange(64, 7680);
    customHeightSpin_->setValue(current.customHeight);

    auto *customSizeRow = new QWidget(this);
    auto *customSizeLayout = new QHBoxLayout(customSizeRow);
    customSizeLayout->setContentsMargins(0, 0, 0, 0);
    customSizeLayout->addWidget(customWidthSpin_);
    customSizeLayout->addWidget(new QLabel(QStringLiteral("x"), this));
    customSizeLayout->addWidget(customHeightSpin_);

    volumeSpin_ = new QSpinBox(this);
    volumeSpin_->setRange(0, 100);
    volumeSpin_->setSuffix(QStringLiteral(" %"));
    volumeSpin_->setValue(current.volume);

    playlistPanelCheck_ = new QCheckBox(QStringLiteral("啟動時顯示播放清單面板"), this);
    playlistPanelCheck_->setChecked(current.playlistPanelVisible);

    repeatModeCombo_ = new QComboBox(this);
    const PlaylistController::RepeatMode allModes[] = {
        PlaylistController::RepeatMode::NoRepeat,
        PlaylistController::RepeatMode::RepeatPlaylist,
        PlaylistController::RepeatMode::RepeatSingle,
        PlaylistController::RepeatMode::PauseAtEnd,
    };
    for (PlaylistController::RepeatMode mode : allModes) {
        repeatModeCombo_->addItem(repeatModeLabel(mode), static_cast<int>(mode));
    }
    repeatModeCombo_->setCurrentIndex(static_cast<int>(current.repeatMode));

    auto *form = new QFormLayout();
    form->addRow(QStringLiteral("啟動預設大小"), sizeModeCombo_);
    form->addRow(QStringLiteral("自訂大小"), customSizeRow);
    form->addRow(QStringLiteral("啟動預設音量"), volumeSpin_);
    form->addRow(QString(), playlistPanelCheck_);
    form->addRow(QStringLiteral("預設影片循環方式"), repeatModeCombo_);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(sizeModeCombo_, &QComboBox::currentIndexChanged, this, &StartupDefaultsDialog::updateCustomSizeEnabled);
    updateCustomSizeEnabled();
}

void StartupDefaultsDialog::updateCustomSizeEnabled() {
    const bool isCustom = sizeModeCombo_->currentIndex() == static_cast<int>(StartupDefaults::SizeMode::Custom);
    customWidthSpin_->setEnabled(isCustom);
    customHeightSpin_->setEnabled(isCustom);
}

StartupDefaults StartupDefaultsDialog::values() const {
    StartupDefaults result;
    result.sizeMode = static_cast<StartupDefaults::SizeMode>(sizeModeCombo_->currentIndex());
    result.customWidth = customWidthSpin_->value();
    result.customHeight = customHeightSpin_->value();
    result.volume = volumeSpin_->value();
    result.playlistPanelVisible = playlistPanelCheck_->isChecked();
    result.repeatMode = static_cast<PlaylistController::RepeatMode>(repeatModeCombo_->currentData().toInt());
    return result;
}
