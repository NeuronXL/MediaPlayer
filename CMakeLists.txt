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
    model/logentry.h
    model/logmodel.cpp
    model/logmodel.h
    service/ffmpeg/ffmpegframeconverter.cpp
    service/ffmpeg/ffmpegframeconverter.h
    service/ffmpeg/ffmpegdecoderworker.cpp
    service/ffmpeg/ffmpegdecoderworker.h
    service/ffmpeg/ffmpegmediadecoder.cpp
    service/ffmpeg/ffmpegmediadecoder.h
    service/ffmpeg/ffmpegutils.cpp
    service/ffmpeg/ffmpegutils.h
    service/logging/logservice.cpp
    service/logging/logservice.h
    service/player/mediaplayerengine.cpp
    service/player/mediaplayerengine.h
    view/logwidget.cpp
    view/logwidget.h
    view/logwidget.ui
    view/mainwindow.cpp
    view/mainwindow.h
    view/mainwindow.ui
    view/videowidget.cpp
    view/videowidget.h
    view/videowidget.ui
    viewmodel/mainwindowviewmodel.cpp
    viewmodel/mainwindowviewmodel.h
)

target_link_libraries(MediaPlayer PRIVATE
    Qt6::Widgets
    PkgConfig::FFMPEG
)
