#include "ffmpegmediadecoder.h"

#include <QFileInfo>

#include "../logging/logservice.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

FFmpegMediaDecoder::FFmpegMediaDecoder(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
{
}

FFmpegMediaDecoder::~FFmpegMediaDecoder() = default;

QString FFmpegMediaDecoder::currentMediaPath() const
{
    return m_currentMediaPath;
}

bool FFmpegMediaDecoder::hasOpenedMedia() const
{
    return !m_currentMediaPath.isEmpty();
}

void FFmpegMediaDecoder::openMedia(const QString& filePath)
{
    if (filePath.isEmpty()) {
        emit mediaOpenFailed(filePath, tr("File path is empty."));
        return;
    }

    emit mediaOpenStarted(filePath);

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit mediaOpenFailed(filePath, tr("File does not exist."));
        return;
    }

    if (!fileInfo.isFile()) {
        emit mediaOpenFailed(filePath, tr("Selected path is not a file."));
        return;
    }

    if (!fileInfo.isReadable()) {
        emit mediaOpenFailed(filePath, tr("File is not readable."));
        return;
    }

    if (!isSupportedVideoFile(filePath)) {
        emit mediaOpenFailed(filePath, tr("File type is not supported."));
        return;
    }

    if (m_currentMediaPath == filePath) {
        emit mediaOpened(filePath);
        return;
    }

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

    auto cleanup = [&]() {
        if (frame != nullptr) {
            av_frame_free(&frame);
        }
        if (packet != nullptr) {
            av_packet_free(&packet);
        }
        if (codecContext != nullptr) {
            avcodec_free_context(&codecContext);
        }
        if (formatContext != nullptr) {
            avformat_close_input(&formatContext);
        }
    };

    int ret = avformat_open_input(&formatContext,
                                  filePath.toStdString().c_str(),
                                  nullptr,
                                  nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("avformat_open_input failed."));
        cleanup();
        return;
    }

    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("avformat_find_stream_info failed."));
        cleanup();
        return;
    }

    av_dump_format(formatContext, 0, filePath.toStdString().c_str(), 0);

    const int videoStreamIndex =
        av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex < 0) {
        emit mediaOpenFailed(filePath, tr("No video stream found."));
        cleanup();
        return;
    }

    AVStream* videoStream = formatContext->streams[videoStreamIndex];
    const AVCodec* codec =
        avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (codec == nullptr) {
        emit mediaOpenFailed(filePath, tr("Failed to find video decoder."));
        cleanup();
        return;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (codecContext == nullptr) {
        emit mediaOpenFailed(filePath, tr("Failed to allocate codec context."));
        cleanup();
        return;
    }

    ret = avcodec_parameters_to_context(codecContext, videoStream->codecpar);
    if (ret < 0) {
        emit mediaOpenFailed(filePath,
                             tr("Failed to copy codec parameters to decoder context."));
        cleanup();
        return;
    }

    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("Failed to open video decoder."));
        cleanup();
        return;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (packet == nullptr || frame == nullptr) {
        emit mediaOpenFailed(filePath, tr("Failed to allocate FFmpeg frame objects."));
        cleanup();
        return;
    }

    bool hasDecodedFirstFrame = false;

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(codecContext, packet);
        av_packet_unref(packet);

        if (ret < 0) {
            emit mediaOpenFailed(filePath, tr("Failed to send packet to decoder."));
            cleanup();
            return;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(codecContext, frame);

            if (ret == AVERROR(EAGAIN)) {
                break;
            }

            if (ret == AVERROR_EOF) {
                break;
            }

            if (ret < 0) {
                emit mediaOpenFailed(filePath, tr("Failed to receive decoded frame."));
                cleanup();
                return;
            }

            AVFrame* decodedFrame = av_frame_clone(frame);
            if (decodedFrame == nullptr) {
                emit mediaOpenFailed(filePath,
                                     tr("Failed to clone the decoded frame."));
                cleanup();
                return;
            }

            if (m_logService != nullptr) {
                m_logService->append(
                    tr("First decoded frame: width=%1 height=%2 format=%3 pts=%4")
                        .arg(decodedFrame->width)
                        .arg(decodedFrame->height)
                        .arg(decodedFrame->format)
                        .arg(decodedFrame->pts));
            }

            hasDecodedFirstFrame = true;
            emit firstFrameDecoded(decodedFrame);

            av_frame_unref(frame);
            break;
        }

        if (hasDecodedFirstFrame) {
            break;
        }
    }

    if (!hasDecodedFirstFrame) {
        emit mediaOpenFailed(filePath, tr("Failed to decode the first video frame."));
        cleanup();
        return;
    }

    m_currentMediaPath = filePath;
    emit currentMediaPathChanged(m_currentMediaPath);
    emit mediaOpened(m_currentMediaPath);

    cleanup();
}

void FFmpegMediaDecoder::closeMedia()
{
    if (m_currentMediaPath.isEmpty()) {
        return;
    }

    m_currentMediaPath.clear();
    emit currentMediaPathChanged(m_currentMediaPath);
}

bool FFmpegMediaDecoder::isSupportedVideoFile(const QString& filePath) const
{
    static const QStringList supportedExtensions = {
        QStringLiteral("mp4"),  QStringLiteral("mkv"),  QStringLiteral("avi"),
        QStringLiteral("mov"),  QStringLiteral("wmv"),  QStringLiteral("flv"),
        QStringLiteral("webm"), QStringLiteral("m4v"),  QStringLiteral("ts"),
        QStringLiteral("mts"),  QStringLiteral("m2ts"), QStringLiteral("mpeg"),
        QStringLiteral("mpg"),  QStringLiteral("3gp"),  QStringLiteral("ogv")};

    const QString suffix = QFileInfo(filePath).suffix();
    return supportedExtensions.contains(suffix, Qt::CaseInsensitive);
}
