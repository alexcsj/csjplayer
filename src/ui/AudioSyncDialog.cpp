#include "ui/AudioSyncDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

AudioSyncDialog::AudioSyncDialog(int currentMs, QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("調整音訊/視訊同步"));

    delaySpin_ = new QSpinBox(this);
    delaySpin_->setRange(-10000, 10000);
    delaySpin_->setSingleStep(10);
    delaySpin_->setSuffix(QStringLiteral(" ms"));
    delaySpin_->setValue(currentMs);

    auto *form = new QFormLayout();
    form->addRow(QStringLiteral("音訊延遲"), delaySpin_);

    auto *hint = new QLabel(QStringLiteral("正數 = 音訊比視訊慢;負數 = 音訊比視訊快"), this);
    hint->setWordWrap(true);

    writeToFileCheck_ = new QCheckBox(QStringLiteral("同時寫入檔案(另存新檔,讓其他播放器也套用)"), this);

    auto *writeHint =
        new QLabel(QStringLiteral("會用 ffmpeg 重新封裝一份新檔案(原始檔名加上 _synced),不會覆蓋原始檔案。"), this);
    writeHint->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addWidget(writeToFileCheck_);
    layout->addWidget(writeHint);
    layout->addWidget(buttons);
}

int AudioSyncDialog::valueMs() const {
    return delaySpin_->value();
}

bool AudioSyncDialog::writeToFile() const {
    return writeToFileCheck_->isChecked();
}
