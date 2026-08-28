#pragma once

#include <QWidget>

class MpvController;
class MpvGLWidget;
class TransportBar;
class PlaylistController;
class PlaylistPanel;
class TitleBar;
class SpeedController;

// The window runs permanently frameless (Qt::FramelessWindowHint set once
// at startup, never toggled) with a custom TitleBar replacing the native
// one. Transport controls, the playlist panel, and the title bar all float
// as translucent overlays on top of the video (rather than occupying their
// own layout space) so the video always fills the whole window.
//
// F12 (4-corner window snap) is NOT implemented: on Wayland, xdg-shell
// gives clients no way to set an absolute window position at all (drag or
// programmatic), which both interactive corner-snap and a keyboard
// "jump to corner" shortcut would need. Tried and confirmed non-functional
// on this setup; not worth carrying the dead code.
class PlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    ~PlayerWindow() override;

    void openFile(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // Centralized wheel handling (F6/F7): horizontal delta seeks
    // (Ctrl = 5 min steps instead of 1 min), vertical delta adjusts volume.
    // Placed here (not on MpvGLWidget) so it also applies when the cursor
    // is over the translucent title/transport bars, not just the video
    // itself -- an ignored wheel event bubbles up from a child widget to
    // its parent, and PlaylistView (the one place wheel should keep
    // scrolling a list) already accepts its own wheel events, so it never
    // reaches here.
    void wheelEvent(QWheelEvent *event) override;

private:
    // Shared "pick media files" dialog (with the pause-during-dialog and
    // DontUseNativeDialog mitigations); returns the picked paths, or empty
    // if cancelled. Callers decide whether to append or replace.
    QStringList runOpenFilesDialog();

    void openFilesDialog();       // PlaylistPanel's "增加檔案": appends.
    void quickOpenFilesDialog();  // TransportBar's ⏏ icon: replaces the playlist.
    void openFolderDialog();
    void exportPlaylistDialog();
    void importPlaylistDialog();
    void updateRepeatModeLabel();
    void layoutOverlays();
    void toggleMaximizeRestore();

    // Ctrl+/ : hide/show the title bar, transport bar, and playlist panel
    // all at once. Now that the window is permanently frameless, this is
    // just a visibility toggle on our own widgets -- no native-window
    // recreation, so the window position/size is never disturbed.
    void toggleChromeHidden();

    // Alt+1/2/3: resize to a preset, clamped to the current screen's
    // available geometry. Unlike position, size changes are respected on
    // Wayland, so plain resize() works here (no startSystemMove()-style
    // workaround needed).
    void resizeToPreset(const QSize &size);

    // Right-click context menu's "顯示媒體內容" action.
    void showMediaInfo();

    MpvController *mpvController_ = nullptr;
    MpvGLWidget *mpvWidget_ = nullptr;
    TitleBar *titleBar_ = nullptr;
    TransportBar *transportBar_ = nullptr;
    PlaylistController *playlistController_ = nullptr;
    PlaylistPanel *playlistPanel_ = nullptr;
    SpeedController *speedController_ = nullptr;

    bool chromeHidden_ = false;
    bool playlistPanelWasVisibleBeforeHide_ = false;
};
