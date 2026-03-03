#include "mediaplayerengine.h"

extern "C" {
#include <libavutil/frame.h>
}

#include "../ffmpeg/ffmpegmediadecoder.h"
#include "../logging/logservice.h"

MediaPlayerEngine::MediaPlayerEngine(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
    , m_mediaDecoder(new FFmpegMediaDecoder(logService, this))
{
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenStarted, this,
            &MediaPlayerEngine::mediaOpenStarted);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpened, this,
            &MediaPlayerEngine::mediaOpened);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenFailed, this,
            &MediaPlayerEngine::mediaOpenFailed);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::currentMediaPathChanged, this,
            &MediaPlayerEngine::currentMediaPathChanged);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::firstFrameDecoded, this,
            [this](AVFrame* frame) {
                const QImage image = m_frameConverter.toQImage(frame);
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

MediaPlayerEngine::~MediaPlayerEngine() = default;

QString MediaPlayerEngine::currentMediaPath() const
{
    return m_mediaDecoder->currentMediaPath();
}

bool MediaPlayerEngine::hasOpenedMedia() const
{
    return m_mediaDecoder->hasOpenedMedia();
}

void MediaPlayerEngine::openMedia(const QString& filePath)
{
    m_mediaDecoder->openMedia(filePath);
}

void MediaPlayerEngine::closeMedia()
{
    m_mediaDecoder->closeMedia();
}

void MediaPlayerEngine::play()
{
    // TODO: Implement playback loop on top of the decoder.
}

void MediaPlayerEngine::pause()
{
    // TODO: Implement pause state handling.
}

void MediaPlayerEngine::stop()
{
    // TODO: Implement stop and playback reset behavior.
}

void MediaPlayerEngine::seek(qint64 positionMs)
{
    Q_UNUSED(positionMs);
    // TODO: Implement seeking once decode pipeline is in place.
}
