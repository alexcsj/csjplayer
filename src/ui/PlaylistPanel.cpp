#include "ui/PlaylistPanel.h"
#include "ui/PlaylistModel.h"
#include "ui/PlaylistView.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

PlaylistPanel::PlaylistPanel(PlaylistModel *model, QWidget *parent) : QWidget(parent) {
    view_ = new PlaylistView(this);
    view_->setModel(model);

    openFilesButton_ = new QPushButton(QStringLiteral("+"), this);
    openFilesButton_->setToolTip(QStringLiteral("增加檔案"));
    openFolderButton_ = new QPushButton(QStringLiteral("📁+"), this);
    openFolderButton_->setToolTip(QStringLiteral("增加資料夾"));
    removeButton_ = new QPushButton(QStringLiteral("−"), this);
    removeButton_->setToolTip(QStringLiteral("移除選取的項目"));
    exportButton_ = new QPushButton(QStringLiteral("匯出清單"), this);
    importButton_ = new QPushButton(QStringLiteral("匯入清單"), this);
    repeatButton_ = new QPushButton(QStringLiteral("不循環"), this);

    auto *toolbarRow1 = new QHBoxLayout();
    toolbarRow1->addWidget(openFilesButton_);
    toolbarRow1->addWidget(openFolderButton_);
    toolbarRow1->addWidget(removeButton_);

    auto *toolbarRow2 = new QHBoxLayout();
    toolbarRow2->addWidget(exportButton_);
    toolbarRow2->addWidget(importButton_);
    toolbarRow2->addWidget(repeatButton_);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbarRow1);
    layout->addLayout(toolbarRow2);
    layout->addWidget(view_, /*stretch=*/1);

    connect(openFilesButton_, &QPushButton::clicked, this, &PlaylistPanel::openFilesRequested);
    connect(openFolderButton_, &QPushButton::clicked, this, &PlaylistPanel::openFolderRequested);
    connect(removeButton_, &QPushButton::clicked, view_, &PlaylistView::removeSelected);
    connect(repeatButton_, &QPushButton::clicked, this, &PlaylistPanel::repeatToggleRequested);
    connect(exportButton_, &QPushButton::clicked, this, &PlaylistPanel::exportPlaylistRequested);
    connect(importButton_, &QPushButton::clicked, this, &PlaylistPanel::importPlaylistRequested);
}

void PlaylistPanel::setRepeatModeLabel(const QString &text) {
    repeatButton_->setText(text);
}
