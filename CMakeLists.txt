cmake_minimum_required(VERSION 3.21.1)
project(MediaPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale
)

qt_add_executable(MediaPlayer
    main.cpp
    model/debuglogmodel.cpp
    view/mainwindow.cpp
    view/mainwindow.ui
    view/debugwidget.cpp
    view/debugwidget.ui
    viewmodel/mainwindowviewmodel.cpp
)

target_link_libraries(MediaPlayer PRIVATE
    Qt6::Widgets
    PkgConfig::FFMPEG
)
