#include "ui/PlayerWindow.h"
#include "mpv/MpvController.h"
#include "mpv/MpvGLWidget.h"
#include "playback/PlaylistController.h"
#include "playback/SpeedController.h"
#include "ui/PlaylistModel.h"
#include "ui/PlaylistPanel.h"
#include "ui/PlaylistView.h"
#include "ui/TitleBar.h"
#include "ui/TransportBar.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QResizeEvent>
#include <QScreen>
#include <QShortcut>
#include <QWheelEvent>

#include <algorithm>
#include <cstdlib>

namespace {

constexpr double kSeekStepShortSeconds = 60.0;
constexpr double kSeekStepLongSeconds = 300.0;
constexpr int kVolumeStep = 5;

// A short, representative subset (not the full ~23-extension list from
// MediaExtensions) -- the full list on one filter-combo line was wide
// enough to force the whole dialog to a large, non-shrinkable minimum
// width. Anything not in this short list is still openable via "All Files".
QString buildMediaFileDialogFilter() {
    return QStringLiteral(
        "Media Files (*.mp4 *.mkv *.avi *.mov *.webm *.flv *.wmv *.mp3 *.flac *.wav *.m4a);;All Files (*)");
}

// Shared sizing/behavior for every file dialog in the app (open files, add
// folder, export/import playlist), so they all look and resize the same way
// instead of whatever default a given static QFileDialog convenience
// function happens to pick (getExistingDirectory in particular used to come
// out too wide and fixed-size compared to the others).
void prepareFileDialog(QFileDialog &dialog) {
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setSizeGripEnabled(true);
    // Explicit minimum below the default resize() target, so a wide filter
    // combo or long path in the sidebar can't lock the dialog to a large
    // non-shrinkable size the way getExistingDirectory's default used to.
    dialog.setMinimumSize(480, 360);
    dialog.resize(700, 450);
}

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

namespace {

// Frosted-glass look: translucent dark background + subtle border, applied
// via QSS on the overlay widgets themselves (not real desktop-see-through
// window transparency -- see PlayerWindow.h comment).
constexpr const char *kTransportBarStyle = R"(
    TransportBar {
        background-color: rgba(24, 24, 28, 175);
        border-top: 1px solid rgba(255, 255, 255, 35);
    }
    QPushButton {
        color: white;
        background-color: rgba(255, 255, 255, 25);
        border: none;
        border-radius: 4px;
        padding: 4px 10px;
    }
    QPushButton:hover { background-color: rgba(255, 255, 255, 55); }
    QPushButton:disabled { color: rgba(255, 255, 255, 90); background-color: rgba(255, 255, 255, 10); }
    QLabel { color: white; background: transparent; }
)";

constexpr const char *kPlaylistPanelStyle = R"(
    PlaylistPanel {
        background-color: rgba(24, 24, 28, 195);
        border-left: 1px solid rgba(255, 255, 255, 35);
    }
    QPushButton {
        color: white;
        background-color: rgba(255, 255, 255, 25);
        border: none;
        border-radius: 4px;
        padding: 4px 8px;
    }
    QPushButton:hover { background-color: rgba(255, 255, 255, 55); }
    QListView { background-color: transparent; color: white; border: none; }
    QListView::item:selected { background-color: rgba(255, 255, 255, 65); }
)";

constexpr const char *kTitleBarStyle = R"(
    TitleBar {
        background-color: rgba(24, 24, 28, 175);
        border-bottom: 1px solid rgba(255, 255, 255, 35);
    }
    QLabel { color: white; background: transparent; padding-left: 2px; }
    QPushButton#titleBarButton {
        color: white;
        background: transparent;
        border: none;
    }
    QPushButton#titleBarButton:hover { background-color: rgba(255, 255, 255, 40); }
    QPushButton#titleBarCloseButton {
        color: white;
        background: transparent;
        border: none;
    }
    QPushButton#titleBarCloseButton:hover { background-color: rgba(232, 17, 35, 210); }
)";

} // namespace

PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("csjplayer"));
    resize(960, 540);
    setAcceptDrops(true);

    // Permanently frameless: the window is never shown with native
    // decorations, so there is nothing to toggle/recreate at runtime (see
    // TitleBar.h comment for why that used to break window positioning).
    // This must be set before the first show() -- main.cpp shows the
    // window only after construction, so this is early enough.
    setWindowFlag(Qt::FramelessWindowHint, true);

    mpvController_ = new MpvController(this);
    mpvWidget_ = new MpvGLWidget(mpvController_, this);
    titleBar_ = new TitleBar(this);
    transportBar_ = new TransportBar(this);
    playlistController_ = new PlaylistController(mpvController_, this);
    playlistPanel_ = new PlaylistPanel(playlistController_->model(), this);
    playlistPanel_->setVisible(false);
    playlistPanel_->setFixedWidth(280);
    speedController_ = new SpeedController(mpvController_, this);

    // All chrome floats on top of the video (positioned manually in
    // layoutOverlays()/resizeEvent()) instead of occupying their own layout
    // space, so the video always fills the entire window.
    titleBar_->setAttribute(Qt::WA_StyledBackground, true);
    titleBar_->setStyleSheet(QString::fromUtf8(kTitleBarStyle));
    transportBar_->setAttribute(Qt::WA_StyledBackground, true);
    transportBar_->setStyleSheet(QString::fromUtf8(kTransportBarStyle));
    playlistPanel_->setAttribute(Qt::WA_StyledBackground, true);
    playlistPanel_->setStyleSheet(QString::fromUtf8(kPlaylistPanelStyle));
    layoutOverlays();

    // Title bar wiring.
    connect(titleBar_, &TitleBar::minimizeClicked, this, &QWidget::showMinimized);
    connect(titleBar_, &TitleBar::maximizeRestoreClicked, this, &PlayerWindow::toggleMaximizeRestore);
    connect(titleBar_, &TitleBar::closeClicked, this, &QWidget::close);
    connect(playlistController_, &PlaylistController::currentIndexChanged, this, [this](int row) {
        const QString path = playlistController_->model()->pathAt(row);
        titleBar_->setTitleText(QFileInfo(path).fileName());
    });

    // Playback control wiring.
    connect(transportBar_, &TransportBar::playPauseClicked, mpvController_, &MpvController::togglePause);
    connect(transportBar_, &TransportBar::seekRequested, mpvController_, &MpvController::seekAbsolute);
    connect(mpvController_, &MpvController::positionChanged, transportBar_, &TransportBar::setPosition);
    connect(mpvController_, &MpvController::durationChanged, transportBar_, &TransportBar::setDuration);
    connect(mpvController_, &MpvController::pausedChanged, transportBar_, &TransportBar::setPaused);
    connect(mpvController_, &MpvController::volumeChanged, transportBar_, &TransportBar::setVolume);
    connect(mpvController_, &MpvController::mutedChanged, transportBar_, &TransportBar::setMuted);
    connect(transportBar_, &TransportBar::muteToggleClicked, mpvController_, &MpvController::toggleMute);

    // F8 speed/direction wiring.
    connect(transportBar_, &TransportBar::speedMagnitudeSelected, speedController_, &SpeedController::setMagnitude);
    connect(transportBar_, &TransportBar::speedDirectionToggleClicked, speedController_,
            &SpeedController::toggleDirection);
    connect(speedController_, &SpeedController::stateChanged, this, [this]() {
        transportBar_->setSpeedState(speedController_->magnitude(), speedController_->isReverse());
    });
    connect(speedController_, &SpeedController::reverseFallbackEngaged, transportBar_,
            &TransportBar::showReverseFallbackHint);

    // Playlist wiring.
    connect(transportBar_, &TransportBar::previousClicked, playlistController_, &PlaylistController::playPrevious);
    connect(transportBar_, &TransportBar::nextClicked, playlistController_, &PlaylistController::playNext);
    connect(transportBar_, &TransportBar::playlistToggleClicked, this, [this]() {
        playlistPanel_->setVisible(!playlistPanel_->isVisible());
        layoutOverlays();
    });
    connect(transportBar_, &TransportBar::openFilesClicked, this, &PlayerWindow::quickOpenFilesDialog);
    connect(playlistController_, &PlaylistController::currentIndexChanged, this,
            [this](int) { transportBar_->setPlaylistNavigationEnabled(true); });
    connect(playlistController_, &PlaylistController::currentIndexChanged, speedController_,
            &SpeedController::resetToForwardNormal);
    connect(playlistController_, &PlaylistController::repeatModeChanged, this, [this](PlaylistController::RepeatMode) {
        updateRepeatModeLabel();
    });
    connect(playlistPanel_->view(), &PlaylistView::playRequested, playlistController_, &PlaylistController::playIndex);
    connect(playlistPanel_->view(), &PlaylistView::removeRequested, this, [this](QList<int> rows) {
        // Remove highest index first so earlier indices in the batch stay valid.
        std::sort(rows.begin(), rows.end(), std::greater<int>());
        for (int row : rows) {
            playlistController_->removeAt(row);
        }
    });
    connect(playlistPanel_, &PlaylistPanel::openFilesRequested, this, &PlayerWindow::openFilesDialog);
    connect(playlistPanel_, &PlaylistPanel::openFolderRequested, this, &PlayerWindow::openFolderDialog);
    connect(playlistPanel_, &PlaylistPanel::exportPlaylistRequested, this, &PlayerWindow::exportPlaylistDialog);
    connect(playlistPanel_, &PlaylistPanel::importPlaylistRequested, this, &PlayerWindow::importPlaylistDialog);
    connect(playlistPanel_, &PlaylistPanel::repeatToggleRequested, playlistController_,
            &PlaylistController::cycleRepeatMode);

    auto *nextShortcut = new QShortcut(QKeySequence(Qt::Key_PageDown), this);
    nextShortcut->setContext(Qt::WindowShortcut);
    connect(nextShortcut, &QShortcut::activated, playlistController_, &PlaylistController::playNext);

    auto *prevShortcut = new QShortcut(QKeySequence(Qt::Key_PageUp), this);
    prevShortcut->setContext(Qt::WindowShortcut);
    connect(prevShortcut, &QShortcut::activated, playlistController_, &PlaylistController::playPrevious);

    auto *chromeToggleShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    chromeToggleShortcut->setContext(Qt::WindowShortcut);
    connect(chromeToggleShortcut, &QShortcut::activated, this, &PlayerWindow::toggleChromeHidden);

    auto *playPauseShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    playPauseShortcut->setContext(Qt::WindowShortcut);
    connect(playPauseShortcut, &QShortcut::activated, mpvController_, &MpvController::togglePause);

    // F6: seek shortcuts.
    auto *seekForwardShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    seekForwardShortcut->setContext(Qt::WindowShortcut);
    connect(seekForwardShortcut, &QShortcut::activated, this,
            [this]() { mpvController_->seekRelative(kSeekStepShortSeconds); });

    auto *seekBackwardShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    seekBackwardShortcut->setContext(Qt::WindowShortcut);
    connect(seekBackwardShortcut, &QShortcut::activated, this,
            [this]() { mpvController_->seekRelative(-kSeekStepShortSeconds); });

    auto *seekForwardLongShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right), this);
    seekForwardLongShortcut->setContext(Qt::WindowShortcut);
    connect(seekForwardLongShortcut, &QShortcut::activated, this,
            [this]() { mpvController_->seekRelative(kSeekStepLongSeconds); });

    auto *seekBackwardLongShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left), this);
    seekBackwardLongShortcut->setContext(Qt::WindowShortcut);
    connect(seekBackwardLongShortcut, &QShortcut::activated, this,
            [this]() { mpvController_->seekRelative(-kSeekStepLongSeconds); });

    // F7: volume shortcuts.
    auto *volumeUpShortcut = new QShortcut(QKeySequence(Qt::Key_Up), this);
    volumeUpShortcut->setContext(Qt::WindowShortcut);
    connect(volumeUpShortcut, &QShortcut::activated, this,
            [this]() { mpvController_->adjustVolume(kVolumeStep); });

    auto *volumeDownShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    volumeDownShortcut->setContext(Qt::WindowShortcut);
    connect(volumeDownShortcut, &QShortcut::activated, this,
            [this]() { mpvController_->adjustVolume(-kVolumeStep); });

    // F10: A-B loop.
    auto *loopAShortcut = new QShortcut(QKeySequence(Qt::Key_BracketLeft), this);
    loopAShortcut->setContext(Qt::WindowShortcut);
    connect(loopAShortcut, &QShortcut::activated, mpvController_, &MpvController::setLoopA);

    auto *loopBShortcut = new QShortcut(QKeySequence(Qt::Key_BracketRight), this);
    loopBShortcut->setContext(Qt::WindowShortcut);
    connect(loopBShortcut, &QShortcut::activated, mpvController_, &MpvController::setLoopB);

    auto *loopClearShortcut = new QShortcut(QKeySequence(Qt::Key_Backslash), this);
    loopClearShortcut->setContext(Qt::WindowShortcut);
    connect(loopClearShortcut, &QShortcut::activated, mpvController_, &MpvController::clearLoop);

    // F1/Alt+1/2/3: window size presets.
    auto *size1Shortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_1), this);
    size1Shortcut->setContext(Qt::WindowShortcut);
    connect(size1Shortcut, &QShortcut::activated, this, [this]() { resizeToPreset(QSize(960, 540)); });

    auto *size2Shortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_2), this);
    size2Shortcut->setContext(Qt::WindowShortcut);
    connect(size2Shortcut, &QShortcut::activated, this, [this]() { resizeToPreset(QSize(1920, 1080)); });

    auto *size3Shortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_3), this);
    size3Shortcut->setContext(Qt::WindowShortcut);
    connect(size3Shortcut, &QShortcut::activated, this, [this]() { resizeToPreset(QSize(3840, 2160)); });

    // Left+right mouse chord on the video toggles fullscreen (middle-click
    // turned out unreliable on the user's platform/WM).
    auto toggleFullscreen = [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    };
    connect(mpvWidget_, &MpvGLWidget::fullscreenToggleRequested, this, toggleFullscreen);

    auto *fullscreenShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    fullscreenShortcut->setContext(Qt::WindowShortcut);
    connect(fullscreenShortcut, &QShortcut::activated, this, toggleFullscreen);
    auto *fullscreenShortcutNumpad = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Enter), this);
    fullscreenShortcutNumpad->setContext(Qt::WindowShortcut);
    connect(fullscreenShortcutNumpad, &QShortcut::activated, this, toggleFullscreen);
}

PlayerWindow::~PlayerWindow() = default;

void PlayerWindow::openFile(const QString &path) {
    playlistController_->addFiles({path});
}

QStringList PlayerWindow::runOpenFilesDialog() {
    // Two mitigations for a reported freeze/crash when opening the file
    // dialog during active playback: (1) pause while the dialog is up, to
    // cut down on concurrent OpenGL rendering; (2) force Qt's own dialog
    // implementation instead of the native GTK/KDE-portal one, which is a
    // well-known source of GL-context conflicts with Qt OpenGL apps on
    // Linux.
    const bool wasPaused = mpvController_->isPaused();
    mpvController_->setPaused(true);

    QFileDialog dialog(this, QStringLiteral("開啟檔案"), QString(), buildMediaFileDialogFilter());
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    prepareFileDialog(dialog);

    const QStringList paths = dialog.exec() == QDialog::Accepted ? dialog.selectedFiles() : QStringList();
    if (paths.isEmpty()) {
        mpvController_->setPaused(wasPaused);
    }
    return paths;
}

void PlayerWindow::openFilesDialog() {
    const QStringList paths = runOpenFilesDialog();
    if (!paths.isEmpty()) {
        playlistController_->addFiles(paths); // plays the first immediately, unpausing.
    }
}

void PlayerWindow::quickOpenFilesDialog() {
    const QStringList paths = runOpenFilesDialog();
    if (!paths.isEmpty()) {
        playlistController_->replaceWithFiles(paths); // clears the playlist first.
    }
}

void PlayerWindow::exportPlaylistDialog() {
    QFileDialog dialog(this, QStringLiteral("匯出清單"), QStringLiteral("playlist.m3u"),
                        QStringLiteral("M3U Playlist (*.m3u);;All Files (*)"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    prepareFileDialog(dialog);

    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
        playlistController_->exportToFile(dialog.selectedFiles().first());
    }
}

void PlayerWindow::importPlaylistDialog() {
    QFileDialog dialog(this, QStringLiteral("匯入清單"), QString(),
                        QStringLiteral("M3U Playlist (*.m3u);;All Files (*)"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    prepareFileDialog(dialog);

    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
        playlistController_->importFromFile(dialog.selectedFiles().first());
    }
}

void PlayerWindow::openFolderDialog() {
    const bool wasPaused = mpvController_->isPaused();
    mpvController_->setPaused(true);

    QFileDialog dialog(this, QStringLiteral("開啟資料夾"));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    prepareFileDialog(dialog);

    QString dir;
    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
        dir = dialog.selectedFiles().first();
    }
    if (!dir.isEmpty()) {
        playlistController_->addFolder(dir); // only auto-plays (unpausing) if the playlist was empty.
    }
    mpvController_->setPaused(wasPaused);
}

void PlayerWindow::updateRepeatModeLabel() {
    playlistPanel_->setRepeatModeLabel(repeatModeLabel(playlistController_->repeatMode()));
}

void PlayerWindow::toggleChromeHidden() {
    chromeHidden_ = !chromeHidden_;

    if (chromeHidden_) {
        playlistPanelWasVisibleBeforeHide_ = playlistPanel_->isVisible();
        titleBar_->setVisible(false);
        transportBar_->setVisible(false);
        playlistPanel_->setVisible(false);
    } else {
        titleBar_->setVisible(true);
        transportBar_->setVisible(true);
        playlistPanel_->setVisible(playlistPanelWasVisibleBeforeHide_);
    }

    // Just a visibility toggle on our own widgets now -- the window itself
    // is always frameless (see constructor), so there's no native-window
    // recreation and the window position/size is never disturbed by this.
    layoutOverlays();
}

void PlayerWindow::toggleMaximizeRestore() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    titleBar_->setMaximized(isMaximized());
}

void PlayerWindow::resizeToPreset(const QSize &size) {
    QSize target = size;
    if (QScreen *scr = screen()) {
        const QSize avail = scr->availableGeometry().size();
        target = QSize(std::min(target.width(), avail.width()), std::min(target.height(), avail.height()));
    }
    if (isFullScreen() || isMaximized()) {
        showNormal();
    }
    resize(target);
}

void PlayerWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void PlayerWindow::dropEvent(QDropEvent *event) {
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            paths << url.toLocalFile();
        }
    }
    if (!paths.isEmpty()) {
        playlistController_->addDropped(paths);
        event->acceptProposedAction();
    }
}

void PlayerWindow::wheelEvent(QWheelEvent *event) {
    const QPoint angleDelta = event->angleDelta();
    const bool ctrlHeld = event->modifiers().testFlag(Qt::ControlModifier);

    if (angleDelta.x() != 0 && std::abs(angleDelta.x()) >= std::abs(angleDelta.y())) {
        // F6: horizontal wheel tilt seeks; Ctrl scales 1 min -> 5 min.
        // Negated vs. the "obvious" angleDelta().x() sign -- reported as
        // feeling backwards (tilt-right was seeking backward) on the user's
        // hardware/platform.
        const double step = ctrlHeld ? kSeekStepLongSeconds : kSeekStepShortSeconds;
        mpvController_->seekRelative(angleDelta.x() > 0 ? -step : step);
        event->accept();
        return;
    }
    if (angleDelta.y() != 0) {
        // F7: vertical wheel adjusts volume.
        mpvController_->adjustVolume(angleDelta.y() > 0 ? kVolumeStep : -kVolumeStep);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void PlayerWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    layoutOverlays();
}

void PlayerWindow::layoutOverlays() {
    mpvWidget_->setGeometry(rect());

    const int titleHeight = titleBar_->sizeHint().height();
    titleBar_->setGeometry(0, 0, width(), titleHeight);

    const int barHeight = transportBar_->sizeHint().height();
    transportBar_->setGeometry(0, height() - barHeight, width(), barHeight);

    const int panelTop = titleBar_->isVisible() ? titleHeight : 0;
    const int panelWidth = playlistPanel_->width();
    playlistPanel_->setGeometry(width() - panelWidth, panelTop, panelWidth, height() - panelTop - barHeight);

    titleBar_->raise();
    transportBar_->raise();
    playlistPanel_->raise();
}
