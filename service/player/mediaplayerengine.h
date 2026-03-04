#ifndef MEDIAPLAYERENGINE_H
#define MEDIAPLAYERENGINE_H

#include <QImage>
#include <QObject>
#include <QString>
#include <QThread>

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
    void handleMediaOpened(const QString& filePath);
    void handleMediaOpenFailed(const QString& filePath,
                               const QString& reason);
    void handleCurrentMediaPathChanged(const QString& filePath);
    void handlePlaybackPaused();
    void handlePlaybackStarted();

  signals:
    void openMediaRequested(const QString& filePath);
    void closeMediaRequested();
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void seekRequested(qint64 positionMs);

    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void playbackStateChanged(PlaybackState state);
    void firstFrameReady(const QImage& frame);
    void frameReady(const QImage& frame);

  private:
    void setupWorker();
    void setPlaybackState(PlaybackState state);

    LogService* m_logService;
    QThread* m_decoderThread;
    FFmpegDecoderWorker* m_decoderWorker;
    QString m_currentMediaPath;
    bool m_hasOpenedMedia;
    PlaybackState m_playbackState;
};

#endif // MEDIAPLAYERENGINE_H
