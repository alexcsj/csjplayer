#pragma once

#include <QWidget>

class PlaylistView;
class PlaylistModel;
class QPushButton;

// Toggleable side panel: a small two-row toolbar (add file / add folder /
// remove / hide-panel button on row 1, export list / import list / repeat
// mode cycle button on row 2) plus the playlist list itself.
class PlaylistPanel : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistPanel(PlaylistModel *model, QWidget *parent = nullptr);

    PlaylistView *view() const { return view_; }

public slots:
    void setRepeatModeLabel(const QString &text);

signals:
    void openFilesRequested();
    void openFolderRequested();
    void repeatToggleRequested();
    void exportPlaylistRequested();
    void importPlaylistRequested();

    // The panel's own "隱藏" button -- lets PlayerWindow collapse the panel
    // without going through the TransportBar's "清單" toggle button or the
    // right-click menu.
    void hideRequested();

private:
    PlaylistView *view_ = nullptr;
    QPushButton *openFilesButton_ = nullptr;
    QPushButton *openFolderButton_ = nullptr;
    QPushButton *removeButton_ = nullptr;
    QPushButton *repeatButton_ = nullptr;
    QPushButton *exportButton_ = nullptr;
    QPushButton *importButton_ = nullptr;
    QPushButton *hideButton_ = nullptr;
};
