#include "ui/PlaylistModel.h"

#include <QFileInfo>
#include <QFont>

PlaylistModel::PlaylistModel(QObject *parent) : QAbstractListModel(parent) {}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return paths_.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= paths_.size()) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        return QFileInfo(paths_.at(index.row())).fileName();
    case Qt::ToolTipRole:
        return paths_.at(index.row());
    case Qt::FontRole:
        if (index.row() == currentRow_) {
            QFont font;
            font.setBold(true);
            return font;
        }
        return {};
    default:
        return {};
    }
}

void PlaylistModel::appendPaths(const QStringList &paths) {
    if (paths.isEmpty()) {
        return;
    }
    beginInsertRows(QModelIndex(), paths_.size(), paths_.size() + paths.size() - 1);
    paths_.append(paths);
    endInsertRows();
}

void PlaylistModel::removeAt(int row) {
    if (row < 0 || row >= paths_.size()) {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    paths_.removeAt(row);
    if (currentRow_ == row) {
        currentRow_ = -1;
    } else if (currentRow_ > row) {
        --currentRow_;
    }
    endRemoveRows();
}

void PlaylistModel::clear() {
    beginResetModel();
    paths_.clear();
    currentRow_ = -1;
    endResetModel();
}

QString PlaylistModel::pathAt(int row) const {
    if (row < 0 || row >= paths_.size()) {
        return {};
    }
    return paths_.at(row);
}

int PlaylistModel::count() const {
    return paths_.size();
}

void PlaylistModel::setCurrentRow(int row) {
    const int old = currentRow_;
    currentRow_ = row;
    if (old >= 0 && old < paths_.size()) {
        emit dataChanged(index(old), index(old), {Qt::FontRole});
    }
    if (row >= 0 && row < paths_.size()) {
        emit dataChanged(index(row), index(row), {Qt::FontRole});
    }
}
