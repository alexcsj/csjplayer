# Fallback find-module for libmpv on platforms without pkg-config (e.g. Windows).
# Expects MPV_DIR (or the standard CMAKE_PREFIX_PATH) to point at an libmpv dev
# package providing include/mpv/client.h and lib/mpv.lib (or libmpv.dll.a).

find_path(MPV_INCLUDE_DIR
    NAMES mpv/client.h
    HINTS "${MPV_DIR}/include" ENV MPV_DIR
)

find_library(MPV_LIBRARY
    NAMES mpv libmpv mpv-2
    HINTS "${MPV_DIR}/lib" ENV MPV_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPV
    REQUIRED_VARS MPV_LIBRARY MPV_INCLUDE_DIR
)

if(MPV_FOUND)
    set(MPV_INCLUDE_DIRS "${MPV_INCLUDE_DIR}")
    set(MPV_LIBRARIES "${MPV_LIBRARY}")
endif()

mark_as_advanced(MPV_INCLUDE_DIR MPV_LIBRARY)
