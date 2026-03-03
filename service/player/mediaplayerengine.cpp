#include "mediaplayerengine.h"

#include "../ffmpeg/ffmpegdecoderworker.h"
#include "../logging/logservice.h"

MediaPlayerEngine::MediaPlayerEngine(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
    , m_decoderThread(new QThread(this))
    , m_decoderWorker(new FFmpegDecoderWorker(logService))
    , m_hasOpenedMedia(false)
{
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

void MediaPlayerEngine::openMedia(const QString& filePath)
{
    emit openMediaRequested(filePath);
}

void MediaPlayerEngine::closeMedia()
{
    emit closeMediaRequested();
}

void MediaPlayerEngine::play()
{
    emit playRequested();
}

void MediaPlayerEngine::pause()
{
    emit pauseRequested();
}

void MediaPlayerEngine::stop()
{
    emit stopRequested();
}

void MediaPlayerEngine::seek(qint64 positionMs)
{
    emit seekRequested(positionMs);
}

void MediaPlayerEngine::handleMediaOpened(const QString& filePath)
{
    m_hasOpenedMedia = true;
    m_currentMediaPath = filePath;
    emit mediaOpened(filePath);
}

void MediaPlayerEngine::handleMediaOpenFailed(const QString& filePath,
                                              const QString& reason)
{
    m_hasOpenedMedia = false;
    emit mediaOpenFailed(filePath, reason);
}

void MediaPlayerEngine::handleCurrentMediaPathChanged(const QString& filePath)
{
    m_currentMediaPath = filePath;
    if (filePath.isEmpty()) {
        m_hasOpenedMedia = false;
    }

    emit currentMediaPathChanged(filePath);
}
