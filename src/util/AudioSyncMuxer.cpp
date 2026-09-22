#include "util/AudioSyncMuxer.h"

#include <QFileInfo>
#include <QStringList>

#include <cmath>

AudioSyncMuxer::AudioSyncMuxer(QObject *parent) : QObject(parent) {}

QString AudioSyncMuxer::outputPathFor(const QString &inputPath) {
    const QFileInfo info(inputPath);
    const QString dir = info.absolutePath();
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix();

    auto candidateAt = [&](int n) {
        const QString stem = n <= 1 ? QStringLiteral("%1_synced").arg(base)
                                     : QStringLiteral("%1_synced%2").arg(base).arg(n);
        return suffix.isEmpty() ? QStringLiteral("%1/%2").arg(dir, stem)
                                 : QStringLiteral("%1/%2.%3").arg(dir, stem, suffix);
    };

    int n = 1;
    QString candidate = candidateAt(n);
    while (QFileInfo::exists(candidate)) {
        candidate = candidateAt(++n);
    }
    return candidate;
}

void AudioSyncMuxer::start(const QString &inputPath, int offsetMs) {
    outputPath_ = outputPathFor(inputPath);
    const QString offsetArg = QString::number(std::abs(offsetMs) / 1000.0, 'f', 3);

    // ffmpeg's dual-input trick: feed the same file twice, apply -itsoffset
    // to whichever copy should start later, and map video from one copy and
    // audio from the other. Positive offsetMs (audio should lag video) delays
    // the audio-source copy; negative (audio should lead) delays the
    // video-source copy instead -- same relative effect, and avoids relying
    // on negative -itsoffset values, which not every demuxer handles well.
    QStringList args;
    args << QStringLiteral("-y") << QStringLiteral("-i") << inputPath << QStringLiteral("-itsoffset") << offsetArg
         << QStringLiteral("-i") << inputPath;
    if (offsetMs >= 0) {
        args << QStringLiteral("-map") << QStringLiteral("0:v") << QStringLiteral("-map") << QStringLiteral("1:a");
    } else {
        args << QStringLiteral("-map") << QStringLiteral("1:v") << QStringLiteral("-map") << QStringLiteral("0:a");
    }
    args << QStringLiteral("-map") << QStringLiteral("0:s?") << QStringLiteral("-map") << QStringLiteral("0:t?")
         << QStringLiteral("-map_metadata") << QStringLiteral("0") << QStringLiteral("-map_chapters")
         << QStringLiteral("0") << QStringLiteral("-c") << QStringLiteral("copy") << outputPath_;

    process_ = new QProcess(this);
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &AudioSyncMuxer::onProcessFinished);
    connect(process_, &QProcess::errorOccurred, this, &AudioSyncMuxer::onProcessErrorOccurred);
    process_->start(QStringLiteral("ffmpeg"), args);
}

void AudioSyncMuxer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (!process_) {
        return; // Already handled by onProcessErrorOccurred (FailedToStart).
    }
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit finished(outputPath_);
    } else {
        QString message = QString::fromUtf8(process_->readAllStandardError()).trimmed();
        if (message.isEmpty()) {
            message = QStringLiteral("ffmpeg 執行失敗(結束代碼 %1)").arg(exitCode);
        }
        emit failed(message);
    }
    process_->deleteLater();
    process_ = nullptr;
}

void AudioSyncMuxer::onProcessErrorOccurred() {
    if (!process_ || process_->error() != QProcess::FailedToStart) {
        return;
    }
    emit failed(QStringLiteral("找不到 ffmpeg,請確認已安裝並在 PATH 中"));
    process_->deleteLater();
    process_ = nullptr;
}
