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
        service/pipeline/interface/ivideorenderadapter.h
        service/pipeline/interface/iaudiooutputadapter.h
        service/pipeline/interface/isubtitlerenderadapter.h
        service/logging/logservice.cpp
        service/logging/logservice.h
        service/pipeline/factory/ffmpegcomponentfactory.cpp
        service/pipeline/factory/ffmpegcomponentfactory.h
        service/pipeline/interface/ffmpegmediademuxer.cpp
        service/pipeline/interface/ffmpegmediademuxer.h
        service/pipeline/interface/ffmpegvideodecodercomponent.cpp
        service/pipeline/interface/ffmpegvideodecodercomponent.h
        service/pipeline/interface/iframetransformer.h
        service/pipeline/interface/ffmpegvideoframetransformer.h
        service/pipeline/interface/ffmpegvideoframetransformer.cpp
        service/pipeline/interface/ffmpegaudioframetransformer.h
        service/pipeline/interface/ffmpegaudioframetransformer.cpp
        service/pipeline/interface/filemediasource.cpp
        service/pipeline/interface/filemediasource.h
        service/pipeline/interface/ipacketqueue.h
        service/pipeline/interface/blockingpacketqueue.h
        service/pipeline/interface/blockingpacketqueue.cpp
        service/pipeline/interface/iframequeue.h
        service/pipeline/interface/blockingframequeue.h
        service/pipeline/interface/blockingframequeue.cpp
        service/pipeline/factory/filemediasourcefactory.cpp
        service/pipeline/factory/filemediasourcefactory.h
        service/pipeline/factory/ibackendcomponentfactory.h
        service/pipeline/interface/imediademuxer.h
        service/pipeline/interface/imediasource.h
        service/pipeline/interface/decoderinitdata.h
        service/pipeline/interface/ivideodecoder.h
        service/pipeline/interface/ipipelineevents.h
        service/pipeline/interface/imediapipeline.h
        service/pipeline/factory/imediasourcefactory.h
        service/pipeline/factory/ibackendfactory.h
        service/pipeline/factory/mediabackendfactory.cpp
        service/pipeline/factory/mediabackendfactory.h
        service/pipeline/mediasourceinfo.h
        service/pipeline/mediapipelineservice.cpp
        service/pipeline/mediapipelineservice.h
        service/player/mediaplayerengine.cpp
        service/player/mediaplayerengine.h
        service/player/engineevent.h
        service/player/masterclocktype.h
        service/player/imediaclock.h
        service/player/mediaclock.cpp
        service/player/mediaclock.h
        service/player/iplaybackscheduler.h
        service/player/simpleplaybackscheduler.h
        service/player/simpleplaybackscheduler.cpp
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
