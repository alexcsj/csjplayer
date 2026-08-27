#include "playback/PlaylistScanner.h"
#include "util/MediaExtensions.h"

#include <QDir>
#include <QDirIterator>

QStringList PlaylistScanner::scanRecursive(const QString &folderPath) {
    QStringList result;
    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (MediaExtensions::isMediaFile(path)) {
            result.append(path);
        }
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}
