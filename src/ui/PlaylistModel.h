#pragma once

#include <QAbstractListModel>
#include <QStringList>

// Holds the full playlist as absolute file paths. Display text is the file
// basename (QFileInfo); the full path is available via pathAt()/ToolTipRole.
// currentRow_ drives a bold-font highlight for the currently playing entry.
class PlaylistModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit PlaylistModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void appendPaths(const QStringList &paths);
    void clear();
    void removeAt(int row);

    QString pathAt(int row) const;
    QStringList allPaths() const { return paths_; }
    int count() const;

    void setCurrentRow(int row);
    int currentRow() const { return currentRow_; }

private:
    QStringList paths_;
    int currentRow_ = -1;
};
