#include "mpv/MpvController.h"

#include <QDebug>
#include <QMetaObject>
#include <QByteArray>

#include <algorithm>
#include <clocale>
#include <stdexcept>

namespace {

// mpv wants owned, heap C strings for mpv_command; this is a tiny RAII-free
// helper since our commands here are all short-lived synchronous calls.
void checkError(int status, const char *what) {
    if (status < 0) {
        qWarning() << "mpv error in" << what << ":" << mpv_error_string(status);
    }
}

} // namespace

MpvController::MpvController(QObject *parent) : QObject(parent) {
    // mpv refuses to initialize unless LC_NUMERIC is "C"; Qt's QApplication
    // applies the system locale on startup (e.g. zh_TW), which breaks mpv's
    // internal numeric parsing if left as-is.
    std::setlocale(LC_NUMERIC, "C");

    mpv_ = mpv_create();
    if (!mpv_) {
        throw std::runtime_error("failed to create mpv instance");
    }

    // We drive rendering ourselves via the render API; mpv must not open
    // its own window.
    mpv_set_option_string(mpv_, "vo", "libmpv");

    // All input goes through Qt. Without disabling mpv's own keyboard
    // bindings, mpv can silently swallow or double-handle keys like the
    // arrow-seek shortcuts we wire up in later milestones.
    mpv_set_option_string(mpv_, "input-default-bindings", "no");
    mpv_set_option_string(mpv_, "input-vo-keyboard", "no");
    mpv_set_option_string(mpv_, "osc", "no");

    // Keep the mpv core alive at end-of-file instead of uninitializing;
    // PlaylistController (later milestone) decides whether to advance.
    mpv_set_option_string(mpv_, "keep-open", "yes");

    // Favor precise seeking; matters more once reverse playback (M5) and
    // A-B loop (M6) land, but harmless as a baseline default now.
    mpv_set_option_string(mpv_, "hr-seek", "yes");

    if (mpv_initialize(mpv_) < 0) {
        throw std::runtime_error("failed to initialize mpv");
    }

    mpv_set_wakeup_callback(mpv_, &MpvController::onMpvWakeup, this);

    mpv_observe_property(mpv_, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv_, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv_, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv_, 0, "eof-reached", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv_, 0, "volume", MPV_FORMAT_INT64);
    mpv_observe_property(mpv_, 0, "mute", MPV_FORMAT_FLAG);
}

MpvController::~MpvController() {
    destroyRenderContext();
    if (mpv_) {
        mpv_terminate_destroy(mpv_);
    }
}

void MpvController::createRenderContext(void *(*getProcAddress)(void *, const char *)) {
    if (mpvGl_) {
        // Expected to have already been torn down (see MpvGLWidget's
        // QOpenGLContext::aboutToBeDestroyed handler) before a new context
        // comes in. If we still get here, warn loudly rather than silently
        // rendering against a stale context tied to a possibly-dead surface.
        qWarning() << "createRenderContext called with an existing render context still alive";
        return;
    }

    mpv_opengl_init_params glInitParams{};
    glInitParams.get_proc_address = getProcAddress;
    glInitParams.get_proc_address_ctx = nullptr;

    // Deliberately NOT using MPV_RENDER_PARAM_ADVANCED_CONTROL. Its docs are
    // explicit that the render thread must then *never* wait on the mpv
    // core, or "a real deadlock will freeze the mpv core thread forever".
    // In this app rendering (paintGL) and normal client calls (loadFile,
    // setPaused, seek, ...) all happen on the same Qt GUI thread -- there
    // is no separate render thread to give that guarantee. With advanced
    // control on, switching files (loadfile, which reconfigures the video
    // pipeline and can need the render side to cooperate) reproducibly
    // deadlocked the whole app (reported as "stuck / not responding" when
    // using PageUp/PageDown or the prev/next buttons). Without it, mpv
    // falls back to the simpler/safer default render-API behavior at the
    // cost of one extra frame copy per frame (direct rendering is
    // disabled) -- an acceptable tradeoff for a desktop player.
    int advanced_control = 0;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInitParams},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced_control},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    if (mpv_render_context_create(&mpvGl_, mpv_, params) < 0) {
        qWarning() << "failed to create mpv render context";
        mpvGl_ = nullptr;
        return;
    }

    mpv_render_context_set_update_callback(mpvGl_, &MpvController::onMpvRenderUpdate, this);
}

void MpvController::pumpRenderUpdate() {
    if (mpvGl_) {
        mpv_render_context_update(mpvGl_);
    }
}

void MpvController::destroyRenderContext() {
    if (mpvGl_) {
        mpv_render_context_set_update_callback(mpvGl_, nullptr, nullptr);
        mpv_render_context_free(mpvGl_);
        mpvGl_ = nullptr;
    }
}

void MpvController::loadFile(const QString &path) {
    QByteArray utf8 = path.toUtf8();
    const char *args[] = {"loadfile", utf8.constData(), nullptr};
    checkError(mpv_command(mpv_, args), "loadfile");
}

void MpvController::setPaused(bool paused) {
    int flag = paused ? 1 : 0;
    checkError(mpv_set_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag), "set pause");
}

void MpvController::togglePause() {
    int flag = 0;
    mpv_get_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag);
    setPaused(!flag);
}

void MpvController::seekAbsolute(double seconds) {
    QByteArray secondsStr = QByteArray::number(seconds, 'f', 3);
    const char *args[] = {"seek", secondsStr.constData(), "absolute", nullptr};
    checkError(mpv_command(mpv_, args), "seek");
}

void MpvController::seekRelative(double deltaSeconds) {
    QByteArray deltaStr = QByteArray::number(deltaSeconds, 'f', 3);
    const char *args[] = {"seek", deltaStr.constData(), "relative", nullptr};
    checkError(mpv_command(mpv_, args), "seek relative");
}

void MpvController::setVolume(int percent) {
    int64_t clamped = std::clamp(percent, 0, 100);
    checkError(mpv_set_property(mpv_, "volume", MPV_FORMAT_INT64, &clamped), "set volume");
}

void MpvController::adjustVolume(int deltaPercent) {
    int64_t current = 0;
    mpv_get_property(mpv_, "volume", MPV_FORMAT_INT64, &current);
    setVolume(static_cast<int>(current) + deltaPercent);
}

void MpvController::setMuted(bool muted) {
    int flag = muted ? 1 : 0;
    checkError(mpv_set_property(mpv_, "mute", MPV_FORMAT_FLAG, &flag), "set mute");
}

void MpvController::toggleMute() {
    int flag = 0;
    mpv_get_property(mpv_, "mute", MPV_FORMAT_FLAG, &flag);
    setMuted(!flag);
}

void MpvController::setSpeed(double magnitude) {
    checkError(mpv_set_property(mpv_, "speed", MPV_FORMAT_DOUBLE, &magnitude), "set speed");
}

void MpvController::setPlayDirectionBackward(bool backward) {
    // "play-dir" is a deprecated alias mpv warns about ("might be removed
    // in the future"); "play-direction" is the current name.
    checkError(mpv_set_property_string(mpv_, "play-direction", backward ? "backward" : "forward"),
               "set play-direction");
}

void MpvController::setLoopA() {
    double pos = 0;
    if (mpv_get_property(mpv_, "time-pos", MPV_FORMAT_DOUBLE, &pos) >= 0) {
        checkError(mpv_set_property(mpv_, "ab-loop-a", MPV_FORMAT_DOUBLE, &pos), "set ab-loop-a");
    }
}

void MpvController::setLoopB() {
    double pos = 0;
    if (mpv_get_property(mpv_, "time-pos", MPV_FORMAT_DOUBLE, &pos) >= 0) {
        checkError(mpv_set_property(mpv_, "ab-loop-b", MPV_FORMAT_DOUBLE, &pos), "set ab-loop-b");
    }
}

void MpvController::clearLoop() {
    checkError(mpv_set_property_string(mpv_, "ab-loop-a", "no"), "clear ab-loop-a");
    checkError(mpv_set_property_string(mpv_, "ab-loop-b", "no"), "clear ab-loop-b");
}

bool MpvController::isPaused() const {
    int flag = 0;
    mpv_get_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag);
    return flag != 0;
}

// Fires on an mpv-internal render thread. Must stay minimal: no mpv_* calls,
// no Qt widget access. We only ever hop to the GUI thread.
void MpvController::onMpvRenderUpdate(void *ctx) {
    auto *self = static_cast<MpvController *>(ctx);
    QMetaObject::invokeMethod(self, [self]() { emit self->frameReady(); }, Qt::QueuedConnection);
}

// Fires on an mpv-internal worker thread. Same rule: only hop to GUI thread.
void MpvController::onMpvWakeup(void *ctx) {
    auto *self = static_cast<MpvController *>(ctx);
    QMetaObject::invokeMethod(self, "processMpvEvents", Qt::QueuedConnection);
}

void MpvController::processMpvEvents() {
    while (true) {
        mpv_event *event = mpv_wait_event(mpv_, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handleEvent(event);
    }
}

void MpvController::handleEvent(mpv_event *event) {
    switch (event->event_id) {
    case MPV_EVENT_VIDEO_RECONFIG:
        emit videoParamsChanged();
        break;
    case MPV_EVENT_PROPERTY_CHANGE: {
        auto *prop = static_cast<mpv_event_property *>(event->data);
        if (!prop->data) {
            break;
        }
        const QString name = QString::fromLatin1(prop->name);
        if (name == QLatin1String("time-pos") && prop->format == MPV_FORMAT_DOUBLE) {
            emit positionChanged(*static_cast<double *>(prop->data));
        } else if (name == QLatin1String("duration") && prop->format == MPV_FORMAT_DOUBLE) {
            emit durationChanged(*static_cast<double *>(prop->data));
        } else if (name == QLatin1String("pause") && prop->format == MPV_FORMAT_FLAG) {
            emit pausedChanged(*static_cast<int *>(prop->data) != 0);
        } else if (name == QLatin1String("eof-reached") && prop->format == MPV_FORMAT_FLAG) {
            emit endOfFileReached(*static_cast<int *>(prop->data) != 0);
        } else if (name == QLatin1String("volume") && prop->format == MPV_FORMAT_INT64) {
            emit volumeChanged(static_cast<int>(*static_cast<int64_t *>(prop->data)));
        } else if (name == QLatin1String("mute") && prop->format == MPV_FORMAT_FLAG) {
            emit mutedChanged(*static_cast<int *>(prop->data) != 0);
        }
        break;
    }
    default:
        break;
    }
}
