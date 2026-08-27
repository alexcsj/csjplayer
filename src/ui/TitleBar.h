#pragma once

#include <QWidget>
#include <QPoint>

class QLabel;
class QPushButton;

// Custom title bar replacing the native one now that the window runs
// permanently frameless (Qt::FramelessWindowHint set once at startup, never
// toggled -- toggling it at runtime forced a native-window recreation that
// reset the window position, notably on Wayland where clients can't restore
// an absolute position afterwards). Drag-to-move uses the same
// QWindow::startSystemMove() approach as MpvGLWidget's video-area drag, for
// the same Wayland-compatibility reason.
class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

public slots:
    void setTitleText(const QString &text);
    void setMaximized(bool maximized);

signals:
    void minimizeClicked();
    void maximizeRestoreClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *titleLabel_ = nullptr;
    QPushButton *minimizeButton_ = nullptr;
    QPushButton *maximizeButton_ = nullptr;
    QPushButton *closeButton_ = nullptr;

    bool pendingDrag_ = false;
    QPoint dragStartGlobalPos_;
};
