#pragma once

#include <QIcon>

// The app's icon: a dark filmstrip with a blue play triangle, drawn with
// QPainter rather than shipped as an image asset -- no icon files to manage,
// and it renders crisply at any size QIcon is asked to scale it to.
namespace AppIcon {
QIcon build();
}
