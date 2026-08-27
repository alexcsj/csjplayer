#include "ui/PlayerWindow.h"
#include "util/AppIcon.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(AppIcon::build());
    // Matches the "csjplayer" name in the installed .desktop entry -- this
    // is what GNOME Shell (and most desktop shells) use to associate a
    // running window with its Alt+Tab/Overview icon, rather than reading
    // the in-process window icon directly.
    app.setDesktopFileName(QStringLiteral("csjplayer"));

    PlayerWindow window;
    window.show();

    if (argc > 1) {
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
