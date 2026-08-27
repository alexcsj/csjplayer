#include "mpv/MpvGLWidget.h"
#include "mpv/MpvController.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWindow>

#include <mpv/render_gl.h>

namespace {

constexpr int kResizeMarginPx = 8;

void *getProcAddressQt(void * /*ctx*/, const char *name) {
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (!glContext) {
        return nullptr;
    }
    return reinterpret_cast<void *>(glContext->getProcAddress(name));
}

QCursor cursorForEdges(Qt::Edges edges) {
    const bool topLeftOrBottomRight =
        (edges & Qt::LeftEdge && edges & Qt::TopEdge) || (edges & Qt::RightEdge && edges & Qt::BottomEdge);
    const bool topRightOrBottomLeft =
        (edges & Qt::RightEdge && edges & Qt::TopEdge) || (edges & Qt::LeftEdge && edges & Qt::BottomEdge);
    if (topLeftOrBottomRight) {
        return QCursor(Qt::SizeFDiagCursor);
    }
    if (topRightOrBottomLeft) {
        return QCursor(Qt::SizeBDiagCursor);
    }
    if (edges & Qt::LeftEdge || edges & Qt::RightEdge) {
        return QCursor(Qt::SizeHorCursor);
    }
    if (edges & Qt::TopEdge || edges & Qt::BottomEdge) {
        return QCursor(Qt::SizeVerCursor);
    }
    return QCursor(Qt::ArrowCursor);
}

} // namespace

MpvGLWidget::MpvGLWidget(MpvController *controller, QWidget *parent)
    : QOpenGLWidget(parent), controller_(controller) {
    connect(controller_, &MpvController::frameReady, this, [this]() { update(); });
    connect(controller_, &MpvController::videoParamsChanged, this, [this]() { update(); });
    // Needed so mouseMoveEvent fires with no button held, to update the
    // resize-cursor hint near edges before the user actually presses.
    setMouseTracking(true);
}

void MpvGLWidget::initializeGL() {
    // Qt can recreate this widget's underlying QOpenGLContext at runtime
    // (notably: Qt::WindowStaysOnTopHint/FramelessWindowHint toggles that
    // force the top-level native window to be recreated), which calls
    // initializeGL() again with a brand-new context. mpv's render context
    // is tied to the exact QOpenGLContext it was created with -- rendering
    // against it after that context is gone is undefined behavior (crash).
    // So: tear the old mpv render context down while its owning context is
    // still current/valid, right before that context actually goes away.
    if (QOpenGLContext *ctx = context()) {
        connect(ctx, &QOpenGLContext::aboutToBeDestroyed, this, [this]() {
            makeCurrent();
            controller_->destroyRenderContext();
            doneCurrent();
        });
    }
    controller_->createRenderContext(&getProcAddressQt);
}

Qt::Edges MpvGLWidget::edgesAt(const QPoint &localPos) const {
    Qt::Edges edges;
    if (localPos.x() <= kResizeMarginPx) {
        edges |= Qt::LeftEdge;
    }
    if (localPos.x() >= width() - kResizeMarginPx) {
        edges |= Qt::RightEdge;
    }
    if (localPos.y() <= kResizeMarginPx) {
        edges |= Qt::TopEdge;
    }
    if (localPos.y() >= height() - kResizeMarginPx) {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

void MpvGLWidget::updateHoverCursor(const QPoint &localPos) {
    setCursor(cursorForEdges(edgesAt(localPos)));
}

void MpvGLWidget::mousePressEvent(QMouseEvent *event) {
    // event->buttons() (plural) reports every button currently held,
    // including the one that just triggered this press -- so this fires
    // exactly once, on whichever button completes the L+R chord, regardless
    // of press order.
    if ((event->buttons() & Qt::LeftButton) && (event->buttons() & Qt::RightButton)) {
        pendingLeftDrag_ = false;
        emit fullscreenToggleRequested();
        event->accept();
        return;
    }

    // Dragging/resizing a fullscreen window makes no sense; leave state
    // untouched so a plain click still reaches the base class normally.
    if (event->button() == Qt::LeftButton && !window()->isFullScreen()) {
        const Qt::Edges edges = edgesAt(event->position().toPoint());
        if (edges != Qt::Edges()) {
            // At an edge: hand the interactive resize off to the
            // compositor/WM immediately (see header comment for why this,
            // rather than computing geometry ourselves, is required).
            if (QWindow *handle = window()->windowHandle()) {
                handle->startSystemResize(edges);
            }
            event->accept();
            return;
        }

        // Interior click: don't accept/swallow -- a plain click or the
        // second click of a double-click must still reach the base class
        // for Qt's normal press/release/double-click machinery to work.
        // Whether this becomes a window-move only gets decided once the
        // cursor actually moves past the drag threshold (mouseMoveEvent).
        pendingLeftDrag_ = true;
        dragStartGlobalPos_ = event->globalPosition().toPoint();
    }
    QOpenGLWidget::mousePressEvent(event);
}

void MpvGLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        updateHoverCursor(event->position().toPoint());
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }

    if (pendingLeftDrag_) {
        const QPoint delta = event->globalPosition().toPoint() - dragStartGlobalPos_;
        if (delta.manhattanLength() < QApplication::startDragDistance()) {
            return;
        }
        // Threshold crossed: this is a drag, not a click. Hand off to the
        // compositor/WM's interactive move (see header comment). Qt won't
        // deliver further move/release events for this gesture once it
        // takes over.
        pendingLeftDrag_ = false;
        if (QWindow *handle = window()->windowHandle()) {
            handle->startSystemMove();
        }
        event->accept();
        return;
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void MpvGLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        pendingLeftDrag_ = false;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void MpvGLWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        controller_->togglePause();
        event->accept();
        return;
    }
    QOpenGLWidget::mouseDoubleClickEvent(event);
}

// mpv's own OpenGL renderer already letterboxes/pillarboxes the video to
// preserve its source aspect ratio within whatever FBO size it's given
// (governed by mpv's `keepaspect` property, on by default) -- it sets its
// own glViewport internally based on fbo.w/fbo.h, so a Qt-side sub-rect
// viewport would just get overridden. We therefore hand mpv the full
// widget-sized FBO and let it handle F2's fit-inside/letterbox math itself.
void MpvGLWidget::paintGL() {
    if (!controller_->renderContextReady()) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    // Required every frame when MPV_RENDER_PARAM_ADVANCED_CONTROL is set --
    // see MpvController::pumpRenderUpdate()'s doc comment.
    controller_->pumpRenderUpdate();

    const qreal dpr = devicePixelRatioF();
    const int fboW = static_cast<int>(width() * dpr);
    const int fboH = static_cast<int>(height() * dpr);

    mpv_opengl_fbo fbo{};
    fbo.fbo = static_cast<int>(defaultFramebufferObject());
    fbo.w = fboW;
    fbo.h = fboH;

    int flipY = 1;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    int status = mpv_render_context_render(controller_->renderContext(), params);
    if (status < 0) {
        qWarning() << "mpv_render_context_render failed:" << mpv_error_string(status);
    }
}
