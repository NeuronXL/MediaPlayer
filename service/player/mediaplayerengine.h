#ifndef MEDIAPLAYERENGINE_H
#define MEDIAPLAYERENGINE_H

#include <QImage>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QTimer>

#include "playbackframe.h"
#include "playbackstate.h"

class FFmpegDecoderWorker;
class LogService;

class MediaPlayerEngine : public QObject
{
    Q_OBJECT

  public:
    explicit MediaPlayerEngine(LogService* logService,
                               QObject* parent = nullptr);
    ~MediaPlayerEngine() override;

    QString currentMediaPath() const;
    bool hasOpenedMedia() const;
    PlaybackState playbackState() const;

  public slots:
    void openMedia(const QString& filePath);
    void closeMedia();
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);

  private slots:
    void handleDecodedFrame(const PlaybackFrame& frame);
    void handleEndOfStreamReached();
    void handleMediaOpened(const QString& filePath);
    void handleMediaOpenFailed(const QString& filePath,
                               const QString& reason);
    void handleCurrentMediaPathChanged(const QString& filePath);
    void handlePlaybackTick();
    void handlePlaybackIntervalChanged(int intervalMs);
    void handleSeekFailed(qint64 positionMs, const QString& reason);

  signals:
    void openMediaRequested(const QString& filePath);
    void closeMediaRequested();
    void seekRequested(qint64 positionMs);
    void workerPlaybackStateChanged(PlaybackState state);
    void workerBufferedStateChanged(qint64 bufferedDurationMs,
                                    int bufferedFrameCount);
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void playbackStateChanged(PlaybackState state);
    void firstFrameReady(const QImage& frame);
    void frameReady(const QImage& frame);

  private:
    qint64 bufferedDurationMs() const;
    void resetFrameQueue();
    void scheduleNextPlaybackTick();
    void syncWorkerBufferedState();
    void setPlaybackState(PlaybackState state);

    LogService* m_logService;
    QThread* m_decoderThread;
    QTimer* m_playbackTimer;
    FFmpegDecoderWorker* m_decoderWorker;
    QQueue<PlaybackFrame> m_playbackFrameQueue;
    QString m_currentMediaPath;
    bool m_hasOpenedMedia;
    bool m_endOfStreamPending;
    int m_playbackIntervalMs;
    qint64 m_lastRenderedPtsMs;
    PlaybackState m_playbackState;
};

#endif // MEDIAPLAYERENGINE_H
