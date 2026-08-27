#pragma once

#include <QStringList>

// Recursively scans a folder (including subfolders) for known media files.
class PlaylistScanner {
public:
    static QStringList scanRecursive(const QString &folderPath);
};
