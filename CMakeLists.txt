cmake_minimum_required(VERSION 3.21)
project(csjplayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MPV IMPORTED_TARGET mpv)
endif()
if(NOT MPV_FOUND)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
    find_package(MPV REQUIRED)
endif()

set(SOURCES
    src/main.cpp
    src/mpv/MpvController.cpp
    src/mpv/MpvGLWidget.cpp
    src/playback/PlaylistController.cpp
    src/playback/PlaylistScanner.cpp
    src/playback/SpeedController.cpp
    src/ui/PlayerWindow.cpp
    src/ui/PlaylistModel.cpp
    src/ui/PlaylistPanel.cpp
    src/ui/PlaylistView.cpp
    src/ui/SeekBar.cpp
    src/ui/SpeedControl.cpp
    src/ui/TitleBar.cpp
    src/ui/TransportBar.cpp
    src/ui/VolumeControl.cpp
    src/util/AppIcon.cpp
)

qt_add_resources(SOURCES resources/csjplayer.qrc)

add_executable(csjplayer ${SOURCES})

target_include_directories(csjplayer PRIVATE src)

target_link_libraries(csjplayer PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

if(TARGET PkgConfig::MPV)
    target_link_libraries(csjplayer PRIVATE PkgConfig::MPV)
else()
    target_include_directories(csjplayer PRIVATE ${MPV_INCLUDE_DIRS})
    target_link_libraries(csjplayer PRIVATE ${MPV_LIBRARIES})
endif()

include(GNUInstallDirs)

install(TARGETS csjplayer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES resources/csjplayer.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

foreach(size 48 128 256)
    install(FILES resources/icons/csjplayer-${size}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME csjplayer.png)
endforeach()
