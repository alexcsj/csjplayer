#include "ui/SeekStepSettingsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QSpinBox *makeSecondsSpinBox(int value, QWidget *parent) {
    auto *spin = new QSpinBox(parent);
    spin->setRange(1, 3600);
    spin->setSuffix(QStringLiteral(" 秒"));
    spin->setValue(value);
    return spin;
}
} // namespace

SeekStepSettingsDialog::SeekStepSettingsDialog(const SeekStepSettings &current, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("快轉/回轉時間設定"));

    plainSpin_ = makeSecondsSpinBox(current.plainSeconds, this);
    zSpin_ = makeSecondsSpinBox(current.zSeconds, this);
    xSpin_ = makeSecondsSpinBox(current.xSeconds, this);
    cSpin_ = makeSecondsSpinBox(current.cSeconds, this);
    ctrlSpin_ = makeSecondsSpinBox(current.ctrlSeconds, this);

    auto *form = new QFormLayout();
    form->addRow(QStringLiteral("← / → (無修飾鍵)"), plainSpin_);
    form->addRow(QStringLiteral("Z + ← / →"), zSpin_);
    form->addRow(QStringLiteral("X + ← / →"), xSpin_);
    form->addRow(QStringLiteral("C + ← / →"), cSpin_);
    form->addRow(QStringLiteral("Ctrl + ← / →"), ctrlSpin_);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

SeekStepSettings SeekStepSettingsDialog::values() const {
    SeekStepSettings result;
    result.plainSeconds = plainSpin_->value();
    result.zSeconds = zSpin_->value();
    result.xSeconds = xSpin_->value();
    result.cSeconds = cSpin_->value();
    result.ctrlSeconds = ctrlSpin_->value();
    return result;
}
