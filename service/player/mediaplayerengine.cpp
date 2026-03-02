#include "mediaplayerengine.h"

#include "../ffmpeg/ffmpegmediadecoder.h"

MediaPlayerEngine::MediaPlayerEngine(QObject* parent)
    : QObject(parent)
    , m_mediaDecoder(new FFmpegMediaDecoder(this))
{
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenStarted, this,
            &MediaPlayerEngine::mediaOpenStarted);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpened, this,
            &MediaPlayerEngine::mediaOpened);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::mediaOpenFailed, this,
            &MediaPlayerEngine::mediaOpenFailed);
    connect(m_mediaDecoder, &FFmpegMediaDecoder::currentMediaPathChanged, this,
            &MediaPlayerEngine::currentMediaPathChanged);
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
