#ifndef FFMPEGDECODERWORKER_H
#define FFMPEGDECODERWORKER_H

#include <QImage>
#include <QObject>
#include <QString>
#include <QTimer>

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
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);

  private slots:
    void handleDecodedFrame(AVFrame* frame);
    void handlePlaybackTick();

  signals:
    void playbackPaused();
    void playbackStarted();
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void firstFrameReady(const QImage& frame);
    void frameReady(const QImage& frame);

  private:
    QTimer* m_playbackTimer;
    LogService* m_logService;
    FFmpegMediaDecoder* m_mediaDecoder;
};

#endif // FFMPEGDECODERWORKER_H
