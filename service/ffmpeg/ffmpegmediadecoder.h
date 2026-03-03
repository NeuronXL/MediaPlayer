#ifndef FFMPEGMEDIADECODER_H
#define FFMPEGMEDIADECODER_H

#include <QObject>
#include <QString>

struct AVFrame;
class LogService;

class FFmpegMediaDecoder : public QObject
{
    Q_OBJECT

  public:
    explicit FFmpegMediaDecoder(LogService* logService, QObject* parent = nullptr);
    ~FFmpegMediaDecoder() override;

    QString currentMediaPath() const;
    bool hasOpenedMedia() const;

  public slots:
    void openMedia(const QString& filePath);
    void closeMedia();

  signals:
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);
    void firstFrameDecoded(AVFrame* frame);

  private:
    bool isSupportedVideoFile(const QString& filePath) const;

    LogService* m_logService;
    QString m_currentMediaPath;
};

#endif // FFMPEGMEDIADECODER_H
