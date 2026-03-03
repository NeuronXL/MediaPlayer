#include "mediaplayerengine.h"

#include "../ffmpeg/ffmpegdecoderworker.h"
#include "../logging/logservice.h"

MediaPlayerEngine::MediaPlayerEngine(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
    , m_decoderThread(new QThread(this))
    , m_decoderWorker(new FFmpegDecoderWorker(logService))
    , m_hasOpenedMedia(false)
    , m_playbackState(PlaybackState::Idle)
{
    qRegisterMetaType<PlaybackState>("PlaybackState");
    setupWorker();
}

MediaPlayerEngine::~MediaPlayerEngine()
{
    m_decoderThread->quit();
    m_decoderThread->wait();
}

void MediaPlayerEngine::setupWorker()
{
    m_decoderWorker->moveToThread(m_decoderThread);
    connect(m_decoderThread, &QThread::finished, m_decoderWorker,
            &QObject::deleteLater);

    connect(this, &MediaPlayerEngine::openMediaRequested, m_decoderWorker,
            &FFmpegDecoderWorker::openMedia, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::closeMediaRequested, m_decoderWorker,
            &FFmpegDecoderWorker::closeMedia, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::playRequested, m_decoderWorker,
            &FFmpegDecoderWorker::play, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::pauseRequested, m_decoderWorker,
            &FFmpegDecoderWorker::pause, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::stopRequested, m_decoderWorker,
            &FFmpegDecoderWorker::stop, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::seekRequested, m_decoderWorker,
            &FFmpegDecoderWorker::seek, Qt::QueuedConnection);

    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaOpenStarted, this,
            &MediaPlayerEngine::mediaOpenStarted, Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaOpened, this,
            &MediaPlayerEngine::handleMediaOpened,
            Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaOpenFailed, this,
            &MediaPlayerEngine::handleMediaOpenFailed,
            Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::currentMediaPathChanged, this,
            &MediaPlayerEngine::handleCurrentMediaPathChanged,
            Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::firstFrameReady, this,
            &MediaPlayerEngine::firstFrameReady, Qt::QueuedConnection);

    m_decoderThread->start();
}

QString MediaPlayerEngine::currentMediaPath() const
{
    return m_currentMediaPath;
}

bool MediaPlayerEngine::hasOpenedMedia() const
{
    return m_hasOpenedMedia;
}

PlaybackState MediaPlayerEngine::playbackState() const
{
    return m_playbackState;
}

void MediaPlayerEngine::openMedia(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    setPlaybackState(PlaybackState::Opening);
    emit openMediaRequested(filePath);
}

void MediaPlayerEngine::closeMedia()
{
    m_hasOpenedMedia = false;
    setPlaybackState(PlaybackState::Idle);
    emit closeMediaRequested();
}

void MediaPlayerEngine::play()
{
    if (!m_hasOpenedMedia) {
        return;
    }

    setPlaybackState(PlaybackState::Playing);
    emit playRequested();
}

void MediaPlayerEngine::pause()
{
    if (!m_hasOpenedMedia) {
        return;
    }

    setPlaybackState(PlaybackState::Paused);
    emit pauseRequested();
}

void MediaPlayerEngine::stop()
{
    if (!m_hasOpenedMedia) {
        return;
    }

    setPlaybackState(PlaybackState::Stopped);
    emit stopRequested();
}

void MediaPlayerEngine::seek(qint64 positionMs)
{
    if (!m_hasOpenedMedia) {
        return;
    }

    emit seekRequested(positionMs);
}

void MediaPlayerEngine::handleMediaOpened(const QString& filePath)
{
    m_hasOpenedMedia = true;
    m_currentMediaPath = filePath;
    setPlaybackState(PlaybackState::Ready);
    emit mediaOpened(filePath);
}

void MediaPlayerEngine::handleMediaOpenFailed(const QString& filePath,
                                              const QString& reason)
{
    m_hasOpenedMedia = false;
    setPlaybackState(PlaybackState::Error);
    emit mediaOpenFailed(filePath, reason);
}

void MediaPlayerEngine::handleCurrentMediaPathChanged(const QString& filePath)
{
    m_currentMediaPath = filePath;
    if (filePath.isEmpty()) {
        m_hasOpenedMedia = false;
        setPlaybackState(PlaybackState::Idle);
    }

    emit currentMediaPathChanged(filePath);
}

void MediaPlayerEngine::setPlaybackState(PlaybackState state)
{
    if (m_playbackState == state) {
        return;
    }

    m_playbackState = state;
    emit playbackStateChanged(m_playbackState);
}
