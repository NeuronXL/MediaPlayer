#include "mediaplayerengine.h"

#include <QtGlobal>

#include "../ffmpeg/ffmpegdecoderworker.h"
#include "../logging/logservice.h"

MediaPlayerEngine::MediaPlayerEngine(LogService* logService, QObject* parent)
    : QObject(parent)
    , m_logService(logService)
    , m_decoderThread(new QThread(this))
    , m_playbackTimer(new QTimer(this))
    , m_decoderWorker(new FFmpegDecoderWorker(logService))
    , m_hasOpenedMedia(false)
    , m_endOfStreamPending(false)
    , m_playbackIntervalMs(33)
    , m_lastRenderedPtsMs(-1)
    , m_currentPositionMs(0)
    , m_playbackState(PlaybackState::Idle)
{
    m_playbackTimer->setSingleShot(false);
    m_playbackTimer->setTimerType(Qt::PreciseTimer);

    m_decoderWorker->moveToThread(m_decoderThread);
    connect(m_decoderThread, &QThread::finished, m_decoderWorker,
            &QObject::deleteLater);

    connect(this, &MediaPlayerEngine::openMediaRequested, m_decoderWorker,
            &FFmpegDecoderWorker::openMedia, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::closeMediaRequested, m_decoderWorker,
            &FFmpegDecoderWorker::closeMedia, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::seekRequested, m_decoderWorker,
            &FFmpegDecoderWorker::seek, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::workerPlaybackStateChanged, m_decoderWorker,
            &FFmpegDecoderWorker::setPlaybackState, Qt::QueuedConnection);
    connect(this, &MediaPlayerEngine::workerBufferedStateChanged,
            m_decoderWorker, &FFmpegDecoderWorker::updateBufferedState,
            Qt::QueuedConnection);

    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaOpenStarted, this,
            &MediaPlayerEngine::mediaOpenStarted, Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaOpened, this,
            &MediaPlayerEngine::handleMediaOpened, Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaOpenFailed,
            this, &MediaPlayerEngine::handleMediaOpenFailed,
            Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::mediaInfoReady, this,
            &MediaPlayerEngine::handleMediaInfoReady, Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::currentMediaPathChanged, this,
            &MediaPlayerEngine::handleCurrentMediaPathChanged,
            Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::seekFailed, this,
            &MediaPlayerEngine::handleSeekFailed, Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::frameDecoded, this,
            &MediaPlayerEngine::handleDecodedFrame, Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::endOfStreamReached, this,
            &MediaPlayerEngine::handleEndOfStreamReached,
            Qt::QueuedConnection);
    connect(m_decoderWorker, &FFmpegDecoderWorker::playbackIntervalChanged, this,
            &MediaPlayerEngine::handlePlaybackIntervalChanged,
            Qt::QueuedConnection);
    connect(m_playbackTimer, &QTimer::timeout, this,
            &MediaPlayerEngine::handlePlaybackTick);

    m_decoderThread->start();
}

MediaPlayerEngine::~MediaPlayerEngine()
{
    m_decoderThread->quit();
    m_decoderThread->wait();
}

QString MediaPlayerEngine::currentMediaPath() const
{
    return m_currentMediaPath;
}

bool MediaPlayerEngine::hasOpenedMedia() const
{
    return m_hasOpenedMedia;
}

MediaInfo MediaPlayerEngine::mediaInfo() const
{
    return m_mediaInfo;
}

PlaybackState MediaPlayerEngine::playbackState() const
{
    return m_playbackState;
}

qint64 MediaPlayerEngine::currentPositionMs() const
{
    return m_currentPositionMs;
}

void MediaPlayerEngine::openMedia(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    m_playbackTimer->stop();
    m_mediaInfo = {};
    setPlaybackState(PlaybackState::Opening);
    resetFrameQueue();
    emit mediaInfoChanged(m_mediaInfo);
    emit durationChanged(0);
    emit workerPlaybackStateChanged(m_playbackState);
    emit openMediaRequested(filePath);
}

void MediaPlayerEngine::closeMedia()
{
    m_hasOpenedMedia = false;
    m_playbackTimer->stop();
    m_mediaInfo = {};
    setPlaybackState(PlaybackState::Idle);
    resetFrameQueue();
    emit mediaInfoChanged(m_mediaInfo);
    emit durationChanged(0);
    emit workerPlaybackStateChanged(m_playbackState);
    emit closeMediaRequested();
}

void MediaPlayerEngine::play()
{
    if (!m_hasOpenedMedia || m_playbackState == PlaybackState::Playing) {
        return;
    }

    setPlaybackState(PlaybackState::Playing);
    emit workerPlaybackStateChanged(m_playbackState);
    scheduleNextPlaybackTick();
}

void MediaPlayerEngine::pause()
{
    if (!m_hasOpenedMedia || m_playbackState != PlaybackState::Playing) {
        return;
    }

    m_playbackTimer->stop();
    setPlaybackState(PlaybackState::Paused);
    emit workerPlaybackStateChanged(m_playbackState);
}

void MediaPlayerEngine::stop()
{
    if (!m_hasOpenedMedia) {
        return;
    }

    m_playbackTimer->stop();
    resetFrameQueue();
    setPlaybackState(PlaybackState::Stopped);
    emit workerPlaybackStateChanged(m_playbackState);
    emit seekRequested(0);
}

void MediaPlayerEngine::seek(qint64 positionMs)
{
    if (!m_hasOpenedMedia) {
        return;
    }

    m_playbackTimer->stop();
    resetFrameQueue();
    emit seekRequested(positionMs);
}

void MediaPlayerEngine::handleDecodedFrame(const PlaybackFrame& frame)
{
    const bool wasEmpty = m_playbackFrameQueue.isEmpty();
    m_playbackFrameQueue.enqueue(frame);
    syncWorkerBufferedState();

    if (m_playbackState == PlaybackState::Playing) {
        if (!m_playbackTimer->isActive()) {
            scheduleNextPlaybackTick();
        }
        return;
    }

    if (wasEmpty) {
        m_lastRenderedPtsMs = frame.ptsMs;
        if (m_currentPositionMs != frame.ptsMs) {
            m_currentPositionMs = frame.ptsMs;
            emit currentPositionChanged(m_currentPositionMs);
        }
        emit firstFrameReady(frame.image);
    }
}

void MediaPlayerEngine::handleEndOfStreamReached()
{
    m_endOfStreamPending = true;
    if (m_playbackFrameQueue.isEmpty() &&
        m_playbackState == PlaybackState::Playing) {
        m_playbackTimer->stop();
        setPlaybackState(PlaybackState::Stopped);
        emit workerPlaybackStateChanged(m_playbackState);
    }
}

void MediaPlayerEngine::handleMediaOpened(const QString& filePath)
{
    m_hasOpenedMedia = true;
    m_currentMediaPath = filePath;
    m_endOfStreamPending = false;
    setPlaybackState(PlaybackState::Ready);
    emit mediaOpened(filePath);
    resetFrameQueue();
    emit workerPlaybackStateChanged(m_playbackState);
    emit seekRequested(0);
}

void MediaPlayerEngine::handleMediaOpenFailed(const QString& filePath,
                                              const QString& reason)
{
    m_hasOpenedMedia = false;
    m_playbackTimer->stop();
    m_mediaInfo = {};
    resetFrameQueue();
    setPlaybackState(PlaybackState::Error);
    emit mediaInfoChanged(m_mediaInfo);
    emit durationChanged(0);
    emit workerPlaybackStateChanged(m_playbackState);
    emit mediaOpenFailed(filePath, reason);
}

void MediaPlayerEngine::handleMediaInfoReady(const MediaInfo& mediaInfo)
{
    m_mediaInfo = mediaInfo;
    emit mediaInfoChanged(m_mediaInfo);
    emit durationChanged(m_mediaInfo.durationMs);
}

void MediaPlayerEngine::handleCurrentMediaPathChanged(const QString& filePath)
{
    m_currentMediaPath = filePath;
    if (filePath.isEmpty()) {
        m_hasOpenedMedia = false;
        m_playbackTimer->stop();
        m_mediaInfo = {};
        resetFrameQueue();
        setPlaybackState(PlaybackState::Idle);
        emit mediaInfoChanged(m_mediaInfo);
        emit durationChanged(0);
        emit workerPlaybackStateChanged(m_playbackState);
    }

    emit currentMediaPathChanged(filePath);
}

void MediaPlayerEngine::handlePlaybackTick()
{
    if (m_playbackState != PlaybackState::Playing) {
        m_playbackTimer->stop();
        return;
    }

    if (m_playbackFrameQueue.isEmpty()) {
        m_playbackTimer->stop();
        if (m_endOfStreamPending) {
            setPlaybackState(PlaybackState::Stopped);
            emit workerPlaybackStateChanged(m_playbackState);
        }
        syncWorkerBufferedState();
        return;
    }

    const PlaybackFrame frame = m_playbackFrameQueue.dequeue();
    m_lastRenderedPtsMs = frame.ptsMs;
    if (m_currentPositionMs != frame.ptsMs) {
        m_currentPositionMs = frame.ptsMs;
        emit currentPositionChanged(m_currentPositionMs);
    }
    emit frameReady(frame.image);
    syncWorkerBufferedState();
    scheduleNextPlaybackTick();
}

void MediaPlayerEngine::handlePlaybackIntervalChanged(int intervalMs)
{
    if (intervalMs <= 0) {
        return;
    }

    m_playbackIntervalMs = intervalMs;
    if (m_playbackTimer->isActive()) {
        scheduleNextPlaybackTick();
    }
}

void MediaPlayerEngine::handleSeekFailed(qint64 positionMs, const QString& reason)
{
    m_playbackTimer->stop();
    setPlaybackState(PlaybackState::Paused);
    emit workerPlaybackStateChanged(m_playbackState);

    if (positionMs != 0 || m_currentMediaPath.isEmpty()) {
        return;
    }

    m_hasOpenedMedia = false;
    m_mediaInfo = {};
    resetFrameQueue();
    setPlaybackState(PlaybackState::Error);
    emit mediaInfoChanged(m_mediaInfo);
    emit durationChanged(0);
    emit mediaOpenFailed(m_currentMediaPath, reason);
}

qint64 MediaPlayerEngine::bufferedDurationMs() const
{
    qint64 durationMs = 0;
    for (const PlaybackFrame& frame : m_playbackFrameQueue) {
        durationMs += qMax<qint64>(1, frame.durationMs > 0 ? frame.durationMs
                                                           : m_playbackIntervalMs);
    }
    return durationMs;
}

void MediaPlayerEngine::resetFrameQueue()
{
    m_playbackFrameQueue.clear();
    m_endOfStreamPending = false;
    m_lastRenderedPtsMs = -1;
    if (m_currentPositionMs != 0) {
        m_currentPositionMs = 0;
        emit currentPositionChanged(m_currentPositionMs);
    }
    syncWorkerBufferedState();
}

void MediaPlayerEngine::scheduleNextPlaybackTick()
{
    if (m_playbackState != PlaybackState::Playing) {
        m_playbackTimer->stop();
        return;
    }

    if (m_playbackFrameQueue.isEmpty()) {
        m_playbackTimer->stop();
        return;
    }

    const PlaybackFrame& nextFrame = m_playbackFrameQueue.head();
    int intervalMs = m_playbackIntervalMs;
    if (m_lastRenderedPtsMs >= 0) {
        const qint64 ptsDeltaMs = nextFrame.ptsMs - m_lastRenderedPtsMs;
        if (ptsDeltaMs > 0) {
            intervalMs = static_cast<int>(ptsDeltaMs);
        } else if (nextFrame.durationMs > 0) {
            intervalMs = static_cast<int>(nextFrame.durationMs);
        }
    } else if (nextFrame.durationMs > 0) {
        intervalMs = static_cast<int>(nextFrame.durationMs);
    }

    intervalMs = qMax(1, intervalMs);
    m_playbackTimer->start(intervalMs);
}

void MediaPlayerEngine::syncWorkerBufferedState()
{
    emit workerBufferedStateChanged(bufferedDurationMs(),
                                    m_playbackFrameQueue.size());
}

void MediaPlayerEngine::setPlaybackState(PlaybackState state)
{
    if (m_playbackState == state) {
        return;
    }

    m_playbackState = state;
    emit playbackStateChanged(m_playbackState);
}
