#pragma once

#include <QSet>
#include <QString>

// Known media file extensions, used to filter PlaylistScanner's recursive
// folder scan and to build the Open File dialog's name filter. Extension
// filtering is a pragmatic tradeoff for a *listing* UI: mpv/ffmpeg can
// actually play plenty of files this list doesn't recognize (via container
// probing), but a playlist needs to decide what to show without invoking a
// full demuxer probe per file.
namespace MediaExtensions {

inline const QSet<QString> &knownExtensions() {
    static const QSet<QString> extensions = {
        // video
        QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("avi"),
        QStringLiteral("mov"), QStringLiteral("webm"), QStringLiteral("flv"),
        QStringLiteral("wmv"), QStringLiteral("mpg"), QStringLiteral("mpeg"),
        QStringLiteral("m4v"), QStringLiteral("3gp"), QStringLiteral("ts"),
        QStringLiteral("m2ts"), QStringLiteral("ogv"), QStringLiteral("vob"),
        // audio
        QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("aac"), QStringLiteral("ogg"), QStringLiteral("wma"),
        QStringLiteral("m4a"), QStringLiteral("opus"), QStringLiteral("ape"),
    };
    return extensions;
}

inline bool isMediaFile(const QString &filePath) {
    const int dotIndex = filePath.lastIndexOf(QLatin1Char('.'));
    if (dotIndex < 0) {
        return false;
    }
    return knownExtensions().contains(filePath.mid(dotIndex + 1).toLower());
}

} // namespace MediaExtensions
