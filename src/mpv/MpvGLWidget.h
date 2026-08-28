#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPoint>

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
// Move/resize are done via QWindow::startSystemMove()/startSystemResize(),
// NOT by computing geometry ourselves and calling QWidget::move()/
// setGeometry(): on Wayland, clients cannot reposition their own top-level
// window by setting an absolute position (the protocol deliberately doesn't
// allow it) -- window()->move() is silently a no-op there. startSystemMove/
// Resize instead ask the compositor/window manager to perform the
// interactive drag itself (the same mechanism a native title bar uses
// internally), which works correctly on both Wayland and X11.
class MpvGLWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit MpvGLWidget(MpvController *controller, QWidget *parent = nullptr);

signals:
    // Left+right chord toggles fullscreen (middle-click turned out to not
    // reliably reach the widget on the user's platform/WM, so this replaces
    // it). The widget doesn't own window state itself, so PlayerWindow does
    // the actual showFullScreen()/showNormal() in response to this.
    void fullscreenToggleRequested();

    // Right-click menu's "顯示媒體內容" action.
    void mediaInfoRequested();

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

    // Set on an interior left press; cleared once either the drag threshold
    // is crossed (which hands off to startSystemMove()) or the button is
    // released (a plain click/double-click).
    bool pendingLeftDrag_ = false;
    QPoint dragStartGlobalPos_;
};
