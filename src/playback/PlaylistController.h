#pragma once

#include <QObject>
#include <QStringList>

class MpvController;
class PlaylistModel;

// Mediates between UI intent (open file, open folder, drop, next/prev,
// PageUp/PageDown) and mpv commands, and owns state that outlives any single
// mpv property: playlist order/current index and the repeat mode.
class PlaylistController : public QObject {
    Q_OBJECT

public:
    // PauseAtEnd: play the current file once, then pause on its last frame
    // without advancing -- distinct from NoRepeat, which still auto-advances
    // sequentially through the rest of the playlist.
    enum class RepeatMode { NoRepeat, RepeatPlaylist, RepeatSingle, PauseAtEnd };

    explicit PlaylistController(MpvController *mpvController, QObject *parent = nullptr);

    PlaylistModel *model() const { return model_; }
    RepeatMode repeatMode() const { return repeatMode_; }

public slots:
    // Bulk folder import (recursive scan). Only auto-plays the first entry
    // if the playlist was empty before this call -- browsing/importing a
    // folder shouldn't rudely interrupt whatever is already playing.
    void addFolder(const QString &folderPath);

    // Explicit file selection (Open File dialog). Always plays the first of
    // the newly added files immediately, like double-clicking it would.
    void addFiles(const QStringList &filePaths);

    // Drag & drop: mixed files/folders. Folders are expanded recursively;
    // non-media files are dropped. Same "play the first immediately"
    // semantics as addFiles(), since a drop is also an explicit user pick.
    void addDropped(const QStringList &paths);

    // Quick-open (the TransportBar icon button next to "清單"): clears the
    // existing playlist first, leaving only the newly selected file(s) --
    // distinct from addFiles(), which appends onto whatever's already there.
    void replaceWithFiles(const QStringList &filePaths);

    void removeAt(int row);

    // Playlist export/import as a standard .m3u playlist file (plain text,
    // one absolute path per line), so a saved list can be handed to other
    // players or re-imported here later.
    bool exportToFile(const QString &filePath) const;
    void importFromFile(const QString &filePath);

    void playIndex(int row);
    void playNext();
    void playPrevious();
    void cycleRepeatMode();

signals:
    void repeatModeChanged(RepeatMode mode);
    void currentIndexChanged(int row);

private slots:
    void onEndOfFile(bool eof);

private:
    MpvController *mpvController_ = nullptr;
    PlaylistModel *model_ = nullptr;
    int currentIndex_ = -1;
    RepeatMode repeatMode_ = RepeatMode::NoRepeat;
};
