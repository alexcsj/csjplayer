#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPoint>
#include <QSize>

class MpvController;

// The video surface. Its main job is OpenGL context management and handing
// frames off to libmpv's render API each paintGL(); it never calls mpv_*
// directly (MpvController owns that) and never observes mpv events itself.
//
// It also doubles as the window's drag-to-move/edge-resize handle: with no
// custom title bar yet (that's M7), and the window frequently running
// frameless (Ctrl+/, F11's future "clean mode"), there is otherwise no way
// to move or resize it. Pressing near an edge/corner resizes from that
// edge; pressing elsewhere and dragging moves the whole window. This works
// the same whether or not the native title bar is currently shown.
//
// Move and single-edge resize are done via
// QWindow::startSystemMove()/startSystemResize(), NOT by computing geometry
// ourselves and calling QWidget::move()/setGeometry(): on Wayland, clients
// cannot reposition their own top-level window by setting an absolute
// position (the protocol deliberately doesn't allow it) -- window()->move()
// is silently a no-op there. startSystemMove/Resize instead ask the
// compositor/window manager to perform the interactive drag itself (the
// same mechanism a native title bar uses internally), which works
// correctly on both Wayland and X11.
//
// Corner resize is the one exception: it needs to keep the window's aspect
// ratio locked while dragging, which startSystemResize() can't do (once
// called, the compositor owns the whole gesture and never asks us again).
// So corners are instead resized manually via plain resize() -- which,
// unlike move(), Wayland *does* allow a client to do to itself -- always
// anchored at the window's top-left. That means dragging the top-left/
// top-right/bottom-left corners grows the window towards the bottom-right
// rather than "from" the grabbed corner (an accepted visual tradeoff, since
// actually anchoring at the opposite corner would require repositioning,
// which is the exact operation Wayland blocks).
class MpvGLWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit MpvGLWidget(MpvController *controller, QWidget *parent = nullptr);

    // Thickness (px) of the invisible edge-resize hit-zone frame around the
    // widget. User-adjustable via the window-size-presets dialog.
    void setResizeMarginPx(int px);
    int resizeMarginPx() const { return resizeMarginPx_; }

signals:
    // Left+right chord toggles fullscreen (middle-click turned out to not
    // reliably reach the widget on the user's platform/WM, so this replaces
    // it). The widget doesn't own window state itself, so PlayerWindow does
    // the actual showFullScreen()/showNormal() in response to this.
    void fullscreenToggleRequested();

    // Right-click menu's "顯示媒體內容" action.
    void mediaInfoRequested();

    // Right-click menu's "調整快轉/回轉時間" action.
    void seekStepSettingsRequested();

    // Right-click menu's "調整視窗大小快捷鍵" action.
    void windowSizeSettingsRequested();

protected:
    void initializeGL() override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    Qt::Edges edgesAt(const QPoint &localPos) const;
    void updateHoverCursor(const QPoint &localPos);

    MpvController *controller_ = nullptr;

    // Thickness (px) of the invisible edge-resize hit-zone frame; corner
    // zones scale off this too (see edgesAt()). Default matches
    // WindowSizePresets::resizeMarginPx.
    int resizeMarginPx_ = 5;

    // Set on an interior left press; cleared once either the drag threshold
    // is crossed (which hands off to startSystemMove()) or the button is
    // released (a plain click/double-click).
    bool pendingLeftDrag_ = false;
    QPoint dragStartGlobalPos_;

    // Manual aspect-ratio-locked corner resize state (see class doc comment
    // for why this can't just use startSystemResize() like single edges do).
    bool pendingCornerResize_ = false;
    Qt::Edges cornerEdges_;
    QPoint cornerDragStartGlobalPos_;
    QSize cornerDragStartSize_;
    double cornerAspectRatio_ = 1.0;
};
