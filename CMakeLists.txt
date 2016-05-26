cmake_minimum_required(VERSION 3.0)

set(NAME player)

project(${NAME})

find_package(Threads)
find_package(PkgConfig)

pkg_check_modules(NCURSES REQUIRED
    ncursesw)
include_directories(${NCURSES_INCLUDE_DIRS})

pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil)
include_directories(${FFMPEG_INCLUDE_DIRS})

pkg_check_modules(AO REQUIRED
    ao)
include_directories(${AO_INCLUDE_DIRS})


include_directories(./include)
add_subdirectory(./src)
