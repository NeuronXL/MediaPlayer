#ifndef FFMPEGMEDIADECODER_H
#define FFMPEGMEDIADECODER_H

#include <QObject>
#include <QString>

enum class DecodeFrameResult
{
    NoMedia,
    FrameReady,
    EndOfStream,
    Error
};

struct AVFrame;
struct AVPacket;
struct AVCodecContext;
struct AVFormatContext;
class LogService;

class FFmpegMediaDecoder : public QObject
{
    Q_OBJECT

  public:
    explicit FFmpegMediaDecoder(LogService* logService,
                                QObject* parent = nullptr);
    ~FFmpegMediaDecoder() override;

    QString currentMediaPath() const;
    int frameIntervalMs() const;
    bool hasOpenedMedia() const;
    DecodeFrameResult decodeNextFrame();
    bool seekTo(qint64 positionMs, QString* errorMessage = nullptr);

  public slots:
    void openMedia(const QString& filePath);
    void closeMedia();

  signals:
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void frameDecoded(AVFrame* frame);

  private:
    DecodeFrameResult decodeNextFrame(const QString& filePath);
    bool isSupportedVideoFile(const QString& filePath) const;
    void resetDecoderSession();

    LogService* m_logService;
    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    AVPacket* m_packet;
    AVFrame* m_frame;
    int m_videoStreamIndex;
    int m_frameIntervalMs;
    bool m_isFlushing;
    QString m_currentMediaPath;
};

#endif // FFMPEGMEDIADECODER_H
