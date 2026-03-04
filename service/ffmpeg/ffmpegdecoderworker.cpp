#include "ffmpegdecoderworker.h"

extern "C" {
#include <libavutil/frame.h>
}

#include "ffmpegframeconverter.h"
#include "ffmpegmediadecoder.h"
#include "../logging/logservice.h"

FFmpegDecoderWorker::FFmpegDecoderWorker(LogService* logService,
                                         QObject* parent)
    : QObject(parent)
    , m_hasOpenedMedia(false)
    , m_bufferLoopScheduled(false)
    , m_bufferingEnabled(false)
    , m_endOfStreamReached(false)
    , m_isPlaying(false)
    , m_seekPending(false)
    , m_stopRequested(false)
    , m_bufferedFrameCount(0)
    , m_bufferedDurationMs(0)
    , m_targetPausedBufferDurationMs(400)
    , m_targetPlayingBufferDurationMs(1200)
    , m_logService(logService)
    , m_mediaDecoder(new FFmpegMediaDecoder(logService, this))
{
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenStarted, this,
            &FFmpegDecoderWorker::mediaOpenStarted);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpened, this,
            &FFmpegDecoderWorker::handleMediaOpenedInternal);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenFailed, this,
            &FFmpegDecoderWorker::handleMediaOpenFailedInternal);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::currentMediaPathChanged, this,
            &FFmpegDecoderWorker::handleCurrentMediaPathChangedInternal);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaInfoUpdated, this,
            &FFmpegDecoderWorker::mediaInfoReady);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::frameDecoded, this,
            &FFmpegDecoderWorker::handleDecodedFrame);
}

FFmpegDecoderWorker::~FFmpegDecoderWorker() = default;

void FFmpegDecoderWorker::openMedia(const QString& filePath)
{
    m_stopRequested = true;
    m_bufferingEnabled = false;
    m_seekPending = false;
    m_endOfStreamReached = false;
    resetBufferingState();

    m_mediaDecoder->openMedia(filePath);
}

void FFmpegDecoderWorker::closeMedia()
{
    m_stopRequested = true;
    m_bufferingEnabled = false;
    m_hasOpenedMedia = false;
    m_seekPending = false;
    m_endOfStreamReached = false;
    resetBufferingState();
    m_mediaDecoder->closeMedia();
}

void FFmpegDecoderWorker::requestNextFrame()
{
    scheduleBuffering();
}

void FFmpegDecoderWorker::seek(qint64 positionMs)
{
    if (!m_hasOpenedMedia) {
        return;
    }

    m_stopRequested = false;
    m_seekPending = true;
    m_endOfStreamReached = false;
    m_bufferingEnabled = false;
    resetBufferingState();

    QString errorMessage;
    if (!m_mediaDecoder->seekTo(positionMs, &errorMessage)) {
        if (m_logService != nullptr && !errorMessage.isEmpty()) {
            m_logService->append(errorMessage);
        }
        m_seekPending = false;
        emit seekFailed(positionMs, errorMessage);
        return;
    }

    switch (m_mediaDecoder->decodeNextFrame()) {
    case DecodeFrameResult::FrameReady:
        break;
    case DecodeFrameResult::EndOfStream:
        m_seekPending = false;
        emit seekFailed(positionMs,
                        tr("No decodable frame found at the requested position."));
        return;
    case DecodeFrameResult::NoMedia:
        m_seekPending = false;
        emit seekFailed(positionMs, tr("No media is open."));
        return;
    case DecodeFrameResult::Error:
        m_seekPending = false;
        emit seekFailed(positionMs,
                        tr("Failed to decode a frame at the requested position."));
        return;
    }

    m_seekPending = false;
    m_bufferingEnabled = true;
    scheduleBuffering();
}

void FFmpegDecoderWorker::setPlaybackState(PlaybackState state)
{
    m_isPlaying = (state == PlaybackState::Playing);
    if (m_hasOpenedMedia && m_bufferingEnabled) {
        scheduleBuffering();
    }
}

void FFmpegDecoderWorker::updateBufferedState(qint64 bufferedDurationMs,
                                              int bufferedFrameCount)
{
    m_bufferedDurationMs = qMax<qint64>(0, bufferedDurationMs);
    m_bufferedFrameCount = qMax(0, bufferedFrameCount);
    scheduleBuffering();
}

void FFmpegDecoderWorker::continueBuffering()
{
    m_bufferLoopScheduled = false;

    if (!shouldContinueBuffering()) {
        return;
    }

    constexpr int kDecodeBurstCount = 3;
    int decodedFrames = 0;

    while (decodedFrames < kDecodeBurstCount && shouldContinueBuffering()) {
        switch (m_mediaDecoder->decodeNextFrame()) {
        case DecodeFrameResult::FrameReady:
            ++decodedFrames;
            break;
        case DecodeFrameResult::EndOfStream:
            m_endOfStreamReached = true;
            emit endOfStreamReached();
            return;
        case DecodeFrameResult::NoMedia:
            return;
        case DecodeFrameResult::Error:
            return;
        }
    }

    if (shouldContinueBuffering()) {
        scheduleBuffering();
    }
}

void FFmpegDecoderWorker::handleDecodedFrame(AVFrame* frame, qint64 ptsMs,
                                             qint64 durationMs, bool isKeyFrame)
{
    FFmpegFrameConverter frameConverter;
    const QImage image = frameConverter.toQImage(frame);

    if (m_logService != nullptr) {
        m_logService->append(
            tr("Converted QImage: isNull=%1 size=%2x%3 pts=%4 duration=%5")
                .arg(image.isNull() ? QStringLiteral("true")
                                    : QStringLiteral("false"))
                .arg(image.width())
                .arg(image.height())
                .arg(ptsMs)
                .arg(durationMs));
    }

    av_frame_free(&frame);

    if (image.isNull()) {
        return;
    }

    m_bufferedDurationMs += qMax<qint64>(1, durationMs);
    ++m_bufferedFrameCount;

    emit frameDecoded(PlaybackFrame{
        image,
        ptsMs,
        durationMs,
        isKeyFrame,
    });
}

void FFmpegDecoderWorker::handleMediaOpenedInternal(const QString& filePath)
{
    m_hasOpenedMedia = true;
    m_stopRequested = false;
    m_seekPending = false;
    m_endOfStreamReached = false;
    m_bufferingEnabled = true;
    resetBufferingState();

    emit playbackIntervalChanged(m_mediaDecoder->frameIntervalMs());
    emit mediaOpened(filePath);
    scheduleBuffering();
}

void FFmpegDecoderWorker::handleMediaOpenFailedInternal(const QString& filePath,
                                                        const QString& reason)
{
    m_hasOpenedMedia = false;
    m_bufferingEnabled = false;
    m_seekPending = false;
    m_endOfStreamReached = false;
    m_stopRequested = true;
    resetBufferingState();
    emit mediaOpenFailed(filePath, reason);
}

void FFmpegDecoderWorker::handleCurrentMediaPathChangedInternal(
    const QString& filePath)
{
    if (filePath.isEmpty()) {
        m_hasOpenedMedia = false;
        m_bufferingEnabled = false;
        m_seekPending = false;
        m_endOfStreamReached = false;
        m_stopRequested = true;
        resetBufferingState();
    }

    emit currentMediaPathChanged(filePath);
}

bool FFmpegDecoderWorker::shouldContinueBuffering() const
{
    return m_hasOpenedMedia && m_bufferingEnabled && !m_stopRequested &&
           !m_seekPending && !m_endOfStreamReached &&
           m_bufferedDurationMs < targetBufferedDurationMs();
}

void FFmpegDecoderWorker::resetBufferingState()
{
    m_bufferLoopScheduled = false;
    m_bufferedDurationMs = 0;
    m_bufferedFrameCount = 0;
}

void FFmpegDecoderWorker::scheduleBuffering()
{
    if (!shouldContinueBuffering() || m_bufferLoopScheduled) {
        return;
    }

    m_bufferLoopScheduled = true;
    QTimer::singleShot(0, this, &FFmpegDecoderWorker::continueBuffering);
}

qint64 FFmpegDecoderWorker::targetBufferedDurationMs() const
{
    return m_isPlaying ? m_targetPlayingBufferDurationMs
                       : m_targetPausedBufferDurationMs;
}
