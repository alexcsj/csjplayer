# Maintainer: csj <csj.taiwan@gmail.com>
pkgname=csjplayer
pkgver=0.4.0
pkgrel=1
pkgdesc="A custom FFmpeg (libmpv)-based media player with playlist, A-B loop, variable speed and reverse playback"
arch=('x86_64')
url="https://github.com/alexcsj/csjplayer"
license=('custom')
depends=('qt6-base' 'mpv')
makedepends=('cmake')
source=()
sha256sums=()

build() {
    cmake -B "$srcdir/build" -S "$startdir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build "$srcdir/build"
}

package() {
    DESTDIR="$pkgdir" cmake --install "$srcdir/build"
}
