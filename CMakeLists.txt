cmake_minimum_required(VERSION 3.0)

set(NAME player)

project(${NAME})

find_package(Threads)
find_package(PkgConfig)

pkg_check_modules(NCURSES REQUIRED
    ncursesw)
include_directories(${NCURSES_INCLUDE_DIRS})

pkg_check_modules(GST REQUIRED
    gstreamermm-1.0)
include_directories(${GST_INCLUDE_DIRS})
add_definitions(-Wno-deprecated-declarations)

set(LIBS 
    ${NCURSES_LIBRARIES}
    ${GST_LIBRARIES}
    pthread)

include_directories(./include)
add_subdirectory(./src)
