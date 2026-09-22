#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

// Bakes an audio/video sync offset (set live via AudioSyncDialog) permanently
// into a copy of a media file, by shelling out to `ffmpeg -c copy` (stream
// copy -- no re-encode, so it's fast and lossless). Always writes to a new
// file next to the original (see outputPathFor()); the original is never
// touched, so a failed/interrupted run can't corrupt the user's media.
class AudioSyncMuxer : public QObject {
    Q_OBJECT

public:
    explicit AudioSyncMuxer(QObject *parent = nullptr);

    // Picks a non-colliding "<name>_syncedN.<ext>" path next to inputPath.
    static QString outputPathFor(const QString &inputPath);

    // Starts an async ffmpeg remux; exactly one of finished()/failed() is
    // emitted once it's done. offsetMs follows AudioSyncDialog's convention:
    // positive delays audio relative to video, negative advances it.
    void start(const QString &inputPath, int offsetMs);

signals:
    void finished(const QString &outputPath);
    void failed(const QString &errorMessage);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred();

private:
    QProcess *process_ = nullptr;
    QString outputPath_;
};
