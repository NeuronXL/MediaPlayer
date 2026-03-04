#ifndef FFMPEGDECODERWORKER_H
#define FFMPEGDECODERWORKER_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "../player/playbackframe.h"
#include "../player/playbackstate.h"

struct AVFrame;

class FFmpegMediaDecoder;
class LogService;

class FFmpegDecoderWorker : public QObject
{
    Q_OBJECT

  public:
    explicit FFmpegDecoderWorker(LogService* logService,
                                 QObject* parent = nullptr);
    ~FFmpegDecoderWorker() override;

  public slots:
    void openMedia(const QString& filePath);
    void closeMedia();
    void requestNextFrame();
    void seek(qint64 positionMs);
    void setPlaybackState(PlaybackState state);
    void updateBufferedState(qint64 bufferedDurationMs, int bufferedFrameCount);

  private slots:
    void continueBuffering();
    void handleDecodedFrame(AVFrame* frame, qint64 ptsMs, qint64 durationMs,
                            bool isKeyFrame);
    void handleMediaOpenedInternal(const QString& filePath);
    void handleMediaOpenFailedInternal(const QString& filePath,
                                       const QString& reason);
    void handleCurrentMediaPathChangedInternal(const QString& filePath);

  signals:
    void endOfStreamReached();
    void playbackIntervalChanged(int intervalMs);
    void seekFailed(qint64 positionMs, const QString& reason);
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void frameDecoded(const PlaybackFrame& frame);

  private:
    bool shouldContinueBuffering() const;
    void resetBufferingState();
    void scheduleBuffering();
    qint64 targetBufferedDurationMs() const;

    bool m_hasOpenedMedia;
    bool m_bufferLoopScheduled;
    bool m_bufferingEnabled;
    bool m_endOfStreamReached;
    bool m_isPlaying;
    bool m_seekPending;
    bool m_stopRequested;
    int m_bufferedFrameCount;
    qint64 m_bufferedDurationMs;
    qint64 m_targetPausedBufferDurationMs;
    qint64 m_targetPlayingBufferDurationMs;
    LogService* m_logService;
    FFmpegMediaDecoder* m_mediaDecoder;
};

#endif // FFMPEGDECODERWORKER_H
