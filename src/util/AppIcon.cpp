#include "util/AppIcon.h"

#include <QPainter>
#include <QPixmap>
#include <QPolygonF>

namespace AppIcon {

QIcon build() {
    QPixmap pixmap(256, 256);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Filmstrip body: dark rounded rectangle.
    const QRectF stripRect(24, 48, 208, 160);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 34));
    painter.drawRoundedRect(stripRect, 16, 16);

    // Sprocket holes along the top and bottom edges.
    painter.setBrush(QColor(245, 245, 248));
    constexpr int holeSize = 18;
    const int holeYTop = static_cast<int>(stripRect.top()) + 14;
    const int holeYBottom = static_cast<int>(stripRect.bottom()) - 14 - holeSize;
    for (int i = 0; i < 5; ++i) {
        const int x = static_cast<int>(stripRect.left()) + 20 + i * 38;
        painter.drawRoundedRect(QRect(x, holeYTop, holeSize, holeSize), 4, 4);
        painter.drawRoundedRect(QRect(x, holeYBottom, holeSize, holeSize), 4, 4);
    }

    // Blue play triangle, centered over the strip.
    painter.setBrush(QColor(41, 121, 255));
    const qreal cx = stripRect.center().x();
    const qreal cy = stripRect.center().y();
    QPolygonF triangle;
    triangle << QPointF(cx - 34, cy - 46) << QPointF(cx - 34, cy + 46) << QPointF(cx + 46, cy);
    painter.drawPolygon(triangle);

    painter.end();

    QIcon icon;
    icon.addPixmap(pixmap);
    return icon;
}

} // namespace AppIcon
