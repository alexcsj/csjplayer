#include "playback/PlaylistController.h"
#include "playback/PlaylistScanner.h"
#include "mpv/MpvController.h"
#include "ui/PlaylistModel.h"
#include "util/MediaExtensions.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

PlaylistController::PlaylistController(MpvController *mpvController, QObject *parent)
    : QObject(parent), mpvController_(mpvController) {
    model_ = new PlaylistModel(this);
    connect(mpvController_, &MpvController::endOfFileReached, this, &PlaylistController::onEndOfFile);
}

void PlaylistController::addFolder(const QString &folderPath) {
    const bool wasEmpty = model_->count() == 0;
    const QStringList scanned = PlaylistScanner::scanRecursive(folderPath);
    if (scanned.isEmpty()) {
        return;
    }
    model_->appendPaths(scanned);
    if (wasEmpty) {
        playIndex(0);
    }
}

void PlaylistController::addFiles(const QStringList &filePaths) {
    if (filePaths.isEmpty()) {
        return;
    }
    const int firstNewRow = model_->count();
    model_->appendPaths(filePaths);
    playIndex(firstNewRow);
}

void PlaylistController::addDropped(const QStringList &paths) {
    QStringList expanded;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (info.isDir()) {
            expanded.append(PlaylistScanner::scanRecursive(path));
        } else if (MediaExtensions::isMediaFile(path)) {
            expanded.append(path);
        }
    }
    addFiles(expanded);
}

void PlaylistController::replaceWithFiles(const QStringList &filePaths) {
    if (filePaths.isEmpty()) {
        return;
    }
    model_->clear();
    currentIndex_ = -1;
    model_->appendPaths(filePaths);
    playIndex(0);
}

void PlaylistController::removeAt(int row) {
    if (row < 0 || row >= model_->count()) {
        return;
    }
    model_->removeAt(row);
    if (row < currentIndex_) {
        --currentIndex_;
    } else if (row == currentIndex_) {
        // The currently-playing entry was removed from the list; playback
        // of whatever mpv already has loaded continues untouched, but
        // nothing is highlighted as "current" until the user picks again.
        currentIndex_ = -1;
    }
}

bool PlaylistController::exportToFile(const QString &filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);
    out << "#EXTM3U\n";
    for (const QString &path : model_->allPaths()) {
        out << path << '\n';
    }
    return true;
}

void PlaylistController::importFromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QStringList paths;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        paths << line;
    }
    // Bulk import behaves like addFolder(): only auto-plays if the playlist
    // was empty, so loading a saved list doesn't interrupt current playback.
    const bool wasEmpty = model_->count() == 0;
    if (paths.isEmpty()) {
        return;
    }
    model_->appendPaths(paths);
    if (wasEmpty) {
        playIndex(0);
    }
}

void PlaylistController::playIndex(int row) {
    if (row < 0 || row >= model_->count()) {
        return;
    }
    currentIndex_ = row;
    model_->setCurrentRow(row);
    mpvController_->loadFile(model_->pathAt(row));
    mpvController_->setPaused(false);
    emit currentIndexChanged(row);
}

void PlaylistController::playNext() {
    if (model_->count() == 0) {
        return;
    }
    int next = currentIndex_ + 1;
    if (next >= model_->count()) {
        if (repeatMode_ != RepeatMode::RepeatPlaylist) {
            return;
        }
        next = 0;
    }
    playIndex(next);
}

void PlaylistController::playPrevious() {
    if (model_->count() == 0) {
        return;
    }
    int prev = currentIndex_ - 1;
    if (prev < 0) {
        if (repeatMode_ != RepeatMode::RepeatPlaylist) {
            return;
        }
        prev = model_->count() - 1;
    }
    playIndex(prev);
}

void PlaylistController::cycleRepeatMode() {
    switch (repeatMode_) {
    case RepeatMode::NoRepeat:
        repeatMode_ = RepeatMode::RepeatPlaylist;
        break;
    case RepeatMode::RepeatPlaylist:
        repeatMode_ = RepeatMode::RepeatSingle;
        break;
    case RepeatMode::RepeatSingle:
        repeatMode_ = RepeatMode::PauseAtEnd;
        break;
    case RepeatMode::PauseAtEnd:
        repeatMode_ = RepeatMode::NoRepeat;
        break;
    }
    emit repeatModeChanged(repeatMode_);
}

void PlaylistController::onEndOfFile(bool eof) {
    if (!eof) {
        return;
    }
    switch (repeatMode_) {
    case RepeatMode::RepeatSingle:
        mpvController_->seekAbsolute(0);
        mpvController_->setPaused(false);
        return;
    case RepeatMode::PauseAtEnd:
        // mpv (keep-open=yes) already paused on the last frame; don't advance.
        return;
    default:
        playNext();
    }
}
