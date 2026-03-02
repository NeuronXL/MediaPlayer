#ifndef MEDIAPLAYERENGINE_H
#define MEDIAPLAYERENGINE_H

#include <QObject>
#include <QImage>
#include <QString>

#include "../ffmpeg/ffmpegframeconverter.h"

struct AVFrame;
class FFmpegMediaDecoder;

class MediaPlayerEngine : public QObject
{
    Q_OBJECT

  public:
    explicit MediaPlayerEngine(QObject* parent = nullptr);
    ~MediaPlayerEngine() override;

    QString currentMediaPath() const;
    bool hasOpenedMedia() const;

  public slots:
    void openMedia(const QString& filePath);
    void closeMedia();
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);

  signals:
    void debugMessageGenerated(const QString& message);
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void firstFrameReady(const QImage& frame);

  private:
    FFmpegFrameConverter m_frameConverter;
    FFmpegMediaDecoder* m_mediaDecoder;
};

#endif // MEDIAPLAYERENGINE_H
