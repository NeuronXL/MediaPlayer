#include "ffmpegdecoderworker.h"

extern "C" {
#include <libavutil/frame.h>
}

#include "ffmpegframeconverter.h"
#include "ffmpegmediadecoder.h"
#include "../logging/logservice.h"

FFmpegDecoderWorker::FFmpegDecoderWorker(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
    , m_mediaDecoder(new FFmpegMediaDecoder(logService, this))
{
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenStarted, this,
            &FFmpegDecoderWorker::mediaOpenStarted);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpened, this,
            &FFmpegDecoderWorker::mediaOpened);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenFailed, this,
            &FFmpegDecoderWorker::mediaOpenFailed);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::currentMediaPathChanged, this,
            &FFmpegDecoderWorker::currentMediaPathChanged);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::firstFrameDecoded, this,
            [this](AVFrame* frame) {
                FFmpegFrameConverter frameConverter;
                const QImage image = frameConverter.toQImage(frame);

                if (m_logService != nullptr) {
                    m_logService->append(
                        tr("Converted QImage: isNull=%1 size=%2x%3")
                            .arg(image.isNull() ? QStringLiteral("true")
                                                : QStringLiteral("false"))
                            .arg(image.width())
                            .arg(image.height()));
                }

                av_frame_free(&frame);

                if (!image.isNull()) {
                    emit firstFrameReady(image);
                }
            });
}

FFmpegDecoderWorker::~FFmpegDecoderWorker() = default;

void FFmpegDecoderWorker::openMedia(const QString& filePath)
{
    m_mediaDecoder->openMedia(filePath);
}

void FFmpegDecoderWorker::closeMedia()
{
    m_mediaDecoder->closeMedia();
}

void FFmpegDecoderWorker::play()
{
    // TODO: Implement continuous decode loop.
}

void FFmpegDecoderWorker::pause()
{
    // TODO: Implement pause handling in the decode worker.
}

void FFmpegDecoderWorker::stop()
{
    // TODO: Implement stop handling in the decode worker.
}

void FFmpegDecoderWorker::seek(qint64 positionMs)
{
    Q_UNUSED(positionMs);
    // TODO: Implement seeking in the decode worker.
}
