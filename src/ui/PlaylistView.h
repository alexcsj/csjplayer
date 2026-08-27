#pragma once

#include <QList>
#include <QListView>

// Thin QListView subclass: double-click-to-play, Delete/Backspace (or the
// PlaylistPanel's "-" button, via removeSelected()) to remove the selected
// entr(ies). ExtendedSelection so Shift range-selects and Ctrl toggles
// individual items, matching normal file-manager/list conventions.
class PlaylistView : public QListView {
    Q_OBJECT

public:
    explicit PlaylistView(QWidget *parent = nullptr);

public slots:
    void removeSelected();

signals:
    void playRequested(int row);
    void removeRequested(QList<int> rows);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};
