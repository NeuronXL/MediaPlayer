#include "ffmpegmediadecoder.h"

#include <QFileInfo>
#include <QtGlobal>

#include "../logging/logservice.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
}

FFmpegMediaDecoder::FFmpegMediaDecoder(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_packet(nullptr)
    , m_frame(nullptr)
    , m_videoStreamIndex(-1)
    , m_frameIntervalMs(33)
    , m_isFlushing(false)
{
}

FFmpegMediaDecoder::~FFmpegMediaDecoder()
{
    resetDecoderSession();
}

QString FFmpegMediaDecoder::currentMediaPath() const
{
    return m_currentMediaPath;
}

int FFmpegMediaDecoder::frameIntervalMs() const
{
    return m_frameIntervalMs;
}

bool FFmpegMediaDecoder::hasOpenedMedia() const
{
    return !m_currentMediaPath.isEmpty();
}

bool FFmpegMediaDecoder::seekTo(qint64 positionMs, QString* errorMessage)
{
    if (m_formatContext == nullptr || m_codecContext == nullptr ||
        m_videoStreamIndex < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("No media is open.");
        }
        return false;
    }

    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    const int64_t seekTarget = av_rescale_q(positionMs, AVRational{1, 1000},
                                            videoStream->time_base);

    const int seekResult =
        av_seek_frame(m_formatContext, m_videoStreamIndex, seekTarget,
                      AVSEEK_FLAG_BACKWARD);
    if (seekResult < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("Failed to seek to the requested position.");
        }
        return false;
    }

    avcodec_flush_buffers(m_codecContext);
    m_isFlushing = false;
    return true;
}

DecodeFrameResult FFmpegMediaDecoder::decodeNextFrame()
{
    return decodeNextFrame(m_currentMediaPath);
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

    if (!m_currentMediaPath.isEmpty()) {
        m_currentMediaPath.clear();
        emit currentMediaPathChanged(m_currentMediaPath);
    }

    resetDecoderSession();

    const QByteArray filePathUtf8 = filePath.toUtf8();
    int ret = avformat_open_input(&m_formatContext, filePathUtf8.constData(),
                                  nullptr, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("avformat_open_input failed."));
        resetDecoderSession();
        return;
    }

    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("avformat_find_stream_info failed."));
        resetDecoderSession();
        return;
    }

    av_dump_format(m_formatContext, 0, filePathUtf8.constData(), 0);

    m_videoStreamIndex = av_find_best_stream(
        m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStreamIndex < 0) {
        emit mediaOpenFailed(filePath, tr("No video stream found."));
        resetDecoderSession();
        return;
    }

    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    const AVCodec* codec =
        avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (codec == nullptr) {
        emit mediaOpenFailed(filePath, tr("Failed to find video decoder."));
        resetDecoderSession();
        return;
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (m_codecContext == nullptr) {
        emit mediaOpenFailed(filePath, tr("Failed to allocate codec context."));
        resetDecoderSession();
        return;
    }

    ret = avcodec_parameters_to_context(m_codecContext, videoStream->codecpar);
    if (ret < 0) {
        emit mediaOpenFailed(
            filePath,
            tr("Failed to copy codec parameters to decoder context."));
        resetDecoderSession();
        return;
    }

    ret = avcodec_open2(m_codecContext, codec, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("Failed to open video decoder."));
        resetDecoderSession();
        return;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (m_packet == nullptr || m_frame == nullptr) {
        emit mediaOpenFailed(filePath,
                             tr("Failed to allocate FFmpeg frame objects."));
        resetDecoderSession();
        return;
    }

    AVRational frameRate =
        av_guess_frame_rate(m_formatContext, videoStream, nullptr);
    if (frameRate.num > 0 && frameRate.den > 0) {
        const double fps = av_q2d(frameRate);
        if (fps > 0.0) {
            m_frameIntervalMs = qMax(8, qRound(1000.0 / fps));
        }
    }

    m_currentMediaPath = filePath;
    emit currentMediaPathChanged(m_currentMediaPath);
    emit mediaOpened(m_currentMediaPath);
}

void FFmpegMediaDecoder::closeMedia()
{
    if (m_currentMediaPath.isEmpty()) {
        return;
    }

    resetDecoderSession();
    m_currentMediaPath.clear();
    emit currentMediaPathChanged(m_currentMediaPath);
}

DecodeFrameResult FFmpegMediaDecoder::decodeNextFrame(const QString& filePath)
{
    if (m_formatContext == nullptr || m_codecContext == nullptr ||
        m_packet == nullptr || m_frame == nullptr || m_videoStreamIndex < 0) {
        return DecodeFrameResult::NoMedia;
    }

    while (true) {
        if (!m_isFlushing) {
            const int readResult = av_read_frame(m_formatContext, m_packet);
            if (readResult >= 0) {
                if (m_packet->stream_index != m_videoStreamIndex) {
                    av_packet_unref(m_packet);
                    continue;
                }

                const int sendResult =
                    avcodec_send_packet(m_codecContext, m_packet);
                av_packet_unref(m_packet);
                if (sendResult < 0) {
                    emit mediaOpenFailed(filePath,
                                         tr("Failed to send packet to decoder."));
                    return DecodeFrameResult::Error;
                }
            } else {
                m_isFlushing = true;

                const int flushResult =
                    avcodec_send_packet(m_codecContext, nullptr);
                if (flushResult < 0 && flushResult != AVERROR_EOF) {
                    emit mediaOpenFailed(
                        filePath, tr("Failed to flush the video decoder."));
                    return DecodeFrameResult::Error;
                }
            }
        }

        while (true) {
            const int receiveResult =
                avcodec_receive_frame(m_codecContext, m_frame);

            if (receiveResult == AVERROR(EAGAIN)) {
                break;
            }

            if (receiveResult == AVERROR_EOF) {
                return DecodeFrameResult::EndOfStream;
            }

            if (receiveResult < 0) {
                emit mediaOpenFailed(filePath,
                                     tr("Failed to receive decoded frame."));
                return DecodeFrameResult::Error;
            }

            AVFrame* decodedFrame = av_frame_clone(m_frame);
            if (decodedFrame == nullptr) {
                emit mediaOpenFailed(filePath,
                                     tr("Failed to clone the decoded frame."));
                return DecodeFrameResult::Error;
            }

            const qint64 ptsMs = framePositionMs(decodedFrame);
            const qint64 durationMs = frameDurationMs(decodedFrame);
            const bool isKeyFrame =
                (decodedFrame->flags & AV_FRAME_FLAG_KEY) != 0;

            emit frameDecoded(decodedFrame, ptsMs, durationMs, isKeyFrame);
            av_frame_unref(m_frame);
            return DecodeFrameResult::FrameReady;
        }

        if (m_isFlushing) {
            return DecodeFrameResult::EndOfStream;
        }
    }
}

qint64 FFmpegMediaDecoder::frameDurationMs(const AVFrame* frame) const
{
    if (frame == nullptr || m_formatContext == nullptr || m_videoStreamIndex < 0) {
        return m_frameIntervalMs;
    }

    const AVRational timeBase =
        m_formatContext->streams[m_videoStreamIndex]->time_base;
    if (frame->duration > 0) {
        return qMax<qint64>(
            1, av_rescale_q(frame->duration, timeBase, AVRational{1, 1000}));
    }

    return m_frameIntervalMs;
}

qint64 FFmpegMediaDecoder::framePositionMs(const AVFrame* frame) const
{
    if (frame == nullptr || m_formatContext == nullptr || m_videoStreamIndex < 0) {
        return 0;
    }

    const int64_t timestamp =
        frame->best_effort_timestamp != AV_NOPTS_VALUE
            ? frame->best_effort_timestamp
            : frame->pts;
    if (timestamp == AV_NOPTS_VALUE) {
        return 0;
    }

    const AVRational timeBase =
        m_formatContext->streams[m_videoStreamIndex]->time_base;
    return av_rescale_q(timestamp, timeBase, AVRational{1, 1000});
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

void FFmpegMediaDecoder::resetDecoderSession()
{
    if (m_frame != nullptr) {
        av_frame_free(&m_frame);
    }
    if (m_packet != nullptr) {
        av_packet_free(&m_packet);
    }
    if (m_codecContext != nullptr) {
        avcodec_free_context(&m_codecContext);
    }
    if (m_formatContext != nullptr) {
        avformat_close_input(&m_formatContext);
    }

    m_videoStreamIndex = -1;
    m_frameIntervalMs = 33;
    m_isFlushing = false;
}
