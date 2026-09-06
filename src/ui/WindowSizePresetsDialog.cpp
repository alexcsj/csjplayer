#include "ui/WindowSizePresetsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QSpinBox *makeSizeSpinBox(int value, QWidget *parent) {
    auto *spin = new QSpinBox(parent);
    spin->setRange(64, 7680);
    spin->setValue(value);
    return spin;
}
} // namespace

WindowSizePresetsDialog::WindowSizePresetsDialog(const WindowSizePresets &current, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("視窗尺寸快捷鍵設定"));

    auto *form = new QFormLayout();
    for (int i = 0; i < 5; ++i) {
        widthSpins_[i] = makeSizeSpinBox(current.presets[i].width, this);
        heightSpins_[i] = makeSizeSpinBox(current.presets[i].height, this);

        auto *rowWidget = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(widthSpins_[i]);
        rowLayout->addWidget(new QLabel(QStringLiteral("x"), this));
        rowLayout->addWidget(heightSpins_[i]);

        form->addRow(QStringLiteral("Alt+%1").arg(i + 1), rowWidget);
    }

    resizeMarginSpin_ = new QSpinBox(this);
    resizeMarginSpin_->setRange(1, 100);
    resizeMarginSpin_->setValue(current.resizeMarginPx);
    resizeMarginSpin_->setSuffix(QStringLiteral(" px"));
    form->addRow(QStringLiteral("邊緣縮放判定框粗細"), resizeMarginSpin_);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

WindowSizePresets WindowSizePresetsDialog::values() const {
    WindowSizePresets result;
    for (int i = 0; i < 5; ++i) {
        result.presets[i].width = widthSpins_[i]->value();
        result.presets[i].height = heightSpins_[i]->value();
    }
    result.resizeMarginPx = resizeMarginSpin_->value();
    return result;
}
