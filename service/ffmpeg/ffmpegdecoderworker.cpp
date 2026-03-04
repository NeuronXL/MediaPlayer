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
    , m_playbackTimer(new QTimer(this))
    , m_logService(logService)
    , m_mediaDecoder(new FFmpegMediaDecoder(logService, this))
{
    m_playbackTimer->setSingleShot(false);
    m_playbackTimer->setTimerType(Qt::PreciseTimer);

    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenStarted, this,
            &FFmpegDecoderWorker::mediaOpenStarted);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpened, this,
            &FFmpegDecoderWorker::mediaOpened);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenFailed, this,
            &FFmpegDecoderWorker::mediaOpenFailed);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::currentMediaPathChanged, this,
            &FFmpegDecoderWorker::currentMediaPathChanged);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::frameDecoded, this,
            &FFmpegDecoderWorker::handleDecodedFrame);
    connect(m_playbackTimer, &QTimer::timeout, this,
            &FFmpegDecoderWorker::handlePlaybackTick);
}

FFmpegDecoderWorker::~FFmpegDecoderWorker() = default;

void FFmpegDecoderWorker::openMedia(const QString& filePath)
{
    m_playbackTimer->stop();
    m_mediaDecoder->openMedia(filePath);
}

void FFmpegDecoderWorker::closeMedia()
{
    m_playbackTimer->stop();
    m_mediaDecoder->closeMedia();
}

void FFmpegDecoderWorker::play()
{
    if (!m_mediaDecoder->hasOpenedMedia()) {
        return;
    }

    if (m_playbackTimer->isActive()) {
        return;
    }

    m_playbackTimer->start(m_mediaDecoder->frameIntervalMs());
    emit playbackStarted();
}

void FFmpegDecoderWorker::pause()
{
    if (!m_playbackTimer->isActive()) {
        return;
    }

    m_playbackTimer->stop();
    emit playbackPaused();
}

void FFmpegDecoderWorker::stop()
{
    m_playbackTimer->stop();
}

void FFmpegDecoderWorker::seek(qint64 positionMs)
{
    Q_UNUSED(positionMs);
    // TODO: Implement seeking in the decode worker.
}

void FFmpegDecoderWorker::handleDecodedFrame(AVFrame* frame)
{
    FFmpegFrameConverter frameConverter;
    const QImage image = frameConverter.toQImage(frame);

    if (m_logService != nullptr && !m_playbackTimer->isActive()) {
        m_logService->append(
            tr("Converted QImage: isNull=%1 size=%2x%3")
                .arg(image.isNull() ? QStringLiteral("true")
                                    : QStringLiteral("false"))
                .arg(image.width())
                .arg(image.height()));
    }

    av_frame_free(&frame);

    if (image.isNull()) {
        return;
    }

    if (m_playbackTimer->isActive()) {
        emit frameReady(image);
        return;
    }

    emit firstFrameReady(image);
}

void FFmpegDecoderWorker::handlePlaybackTick()
{
    switch (m_mediaDecoder->decodeNextFrame()) {
    case DecodeFrameResult::FrameReady:
        break;
    case DecodeFrameResult::EndOfStream:
        m_playbackTimer->stop();
        if (m_logService != nullptr) {
            m_logService->append(tr("Reached end of video stream."));
        }
        break;
    case DecodeFrameResult::NoMedia:
        m_playbackTimer->stop();
        break;
    case DecodeFrameResult::Error:
        m_playbackTimer->stop();
        break;
    }
}
