cmake_minimum_required(VERSION 3.21.1)
project(MediaPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

# FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale
    libswresample
)

qt_add_executable(MediaPlayer
    main.cpp
	model/logentry.cpp
    model/logentry.h
    model/logmodel.cpp
    model/logmodel.h
    service/adapter/qimagevideoadapter.cpp
    service/adapter/qimagevideoadapter.h
    service/adapter/qaudiooutputadapter.cpp
    service/adapter/qaudiooutputadapter.h
    service/adapter/iaudioadapter.cpp
    service/adapter/ivideoadapter.h
    service/adapter/ivideoadapter.cpp
    service/logging/logservice.cpp
    service/logging/logservice.h
    service/pipeline/framequeue.h
    service/pipeline/ffmpegdemuxer.cpp
    service/pipeline/ffmpegdemuxer.h
    service/pipeline/ffmpegaudiodecoder.cpp
    service/pipeline/ffmpegaudiodecoder.h
    service/pipeline/ffmpegvideodecoder.cpp
    service/pipeline/ffmpegvideodecoder.h
    service/pipeline/mediapipelineservice.cpp
    service/pipeline/mediapipelineservice.h
    service/pipeline/packetqueue.h
    service/pipeline/workerconfig.h
    service/player/mediaplayerengine.cpp
    service/player/mediaplayerengine.h
    service/player/mediaclock.cpp
    service/player/mediaclock.h
    service/adapter/iaudioadapter.h
    service/player/playstate.h
    view/logwidget.cpp
    view/logwidget.h
    view/logwidget.ui
    view/mainwindow.cpp
    view/mainwindow.h
    view/mainwindow.ui
    view/playbackcontrolwidget.cpp
    view/playbackcontrolwidget.h
    view/playbackcontrolwidget.ui
    view/videowidget.cpp
    view/videowidget.h
    view/videowidget.ui
    viewmodel/mainwindowviewmodel.cpp
    viewmodel/mainwindowviewmodel.h
)

target_link_libraries(MediaPlayer PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    PkgConfig::FFMPEG
)
