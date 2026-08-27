#include "ui/PlaylistView.h"

#include <QKeyEvent>
#include <QWheelEvent>

PlaylistView::PlaylistView(QWidget *parent) : QListView(parent) {
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(this, &QListView::doubleClicked, this,
            [this](const QModelIndex &index) { emit playRequested(index.row()); });
}

void PlaylistView::removeSelected() {
    const QModelIndexList selected = selectionModel() ? selectionModel()->selectedRows() : QModelIndexList();
    if (selected.isEmpty()) {
        return;
    }
    QList<int> rows;
    rows.reserve(selected.size());
    for (const QModelIndex &index : selected) {
        rows.append(index.row());
    }
    emit removeRequested(rows);
}

void PlaylistView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (selectionModel() && selectionModel()->hasSelection()) {
            removeSelected();
            event->accept();
            return;
        }
    }
    QListView::keyPressEvent(event);
}

void PlaylistView::wheelEvent(QWheelEvent *event) {
    // QAbstractScrollArea's default wheelEvent ignores the event once the
    // scrollbar is already at its top/bottom limit, which then lets it
    // bubble up to PlayerWindow and get reinterpreted as a volume/seek
    // wheel gesture. Always claim it here so scrolling past either end of
    // the list just does nothing, instead of leaking into playback control.
    QListView::wheelEvent(event);
    event->accept();
}
