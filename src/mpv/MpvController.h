#pragma once

#include <QObject>
#include <QString>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <optional>

// The only class in the codebase allowed to call mpv_* C API functions
// directly. Owns the mpv_handle and (once MpvGLWidget has a valid GL
// context) the mpv_render_context. All libmpv callbacks land on mpv's
// internal worker threads; this class is responsible for marshaling
// everything back onto the Qt GUI thread via Qt::QueuedConnection before
// touching mpv_handle/mpv_render_context again or emitting signals that
// widgets react to.
class MpvController : public QObject {
    Q_OBJECT

public:
    explicit MpvController(QObject *parent = nullptr);
    ~MpvController() override;

    // Called by MpvGLWidget::initializeGL() once a current GL context exists.
    void createRenderContext(void *(*getProcAddress)(void *, const char *));
    void destroyRenderContext();

    mpv_render_context *renderContext() const { return mpvGl_; }
    bool renderContextReady() const { return mpvGl_ != nullptr; }

    // Called once per paintGL(), on the GUI thread with the mpv GL context
    // current, BEFORE mpv_render_context_render(). Harmless/optional now
    // that MPV_RENDER_PARAM_ADVANCED_CONTROL is off (see createRenderContext
    // for why we stopped using it -- it required this AND required the
    // render thread to never wait on the mpv core, which deadlocked this
    // app since render and normal client calls share one thread here).
    void pumpRenderUpdate();

    void loadFile(const QString &path);

    void setPaused(bool paused);
    void togglePause();
    void seekAbsolute(double seconds);
    void seekRelative(double deltaSeconds);

    void setVolume(int percent);      // clamped to [0, 100]
    void adjustVolume(int deltaPercent);
    void setMuted(bool muted);
    void toggleMute();

    // F8: magnitude (always positive) and direction are independent mpv
    // concepts ("speed" vs. "play-dir") -- kept as two separate calls here
    // rather than one signed-speed call, matching that split 1:1.
    void setSpeed(double magnitude);
    void setPlayDirectionBackward(bool backward);

    // F10: A-B loop, using mpv's native ab-loop-a/ab-loop-b properties
    // directly rather than hand-rolled looping. If the *other* point hasn't
    // been explicitly set yet when one is set, it's auto-filled to the
    // video's start/end (0 / duration) so the loop is immediately active
    // from just one keypress -- but that auto-filled point is not
    // considered "explicit", so loopMarkersChanged() reports it as unset
    // (the seek bar hides its marker; only user-set points get one).
    void setLoopA();
    void setLoopB();
    void clearLoop();

    // Synchronous query, safe to call anytime from the GUI thread.
    bool isPaused() const;

signals:
    // Emitted (GUI thread) whenever mpv has a new frame ready to present.
    void frameReady();
    void videoParamsChanged();

    // Property-change notifications, forwarded from mpv_observe_property.
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void pausedChanged(bool paused);
    void endOfFileReached(bool eof);
    void volumeChanged(int percent);
    void mutedChanged(bool muted);

    // nullopt means "no marker to show" (either never set, or auto-filled
    // rather than explicitly chosen by the user -- see setLoopA()/setLoopB()).
    void loopMarkersChanged(std::optional<double> loopAExplicitSeconds, std::optional<double> loopBExplicitSeconds);

private slots:
    // Invoked via Qt::QueuedConnection from the mpv wakeup callback.
    void processMpvEvents();

private:
    static void onMpvRenderUpdate(void *ctx);
    static void onMpvWakeup(void *ctx);

    void handleEvent(mpv_event *event);

    mpv_handle *mpv_ = nullptr;
    mpv_render_context *mpvGl_ = nullptr;

    std::optional<double> loopAExplicitPos_;
    std::optional<double> loopBExplicitPos_;
};
