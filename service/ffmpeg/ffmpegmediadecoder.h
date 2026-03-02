#ifndef FFMPEGMEDIADECODER_H
#define FFMPEGMEDIADECODER_H

#include <QObject>
#include <QString>

class FFmpegMediaDecoder : public QObject
{
    Q_OBJECT

  public:
    explicit FFmpegMediaDecoder(QObject* parent = nullptr);
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

  private:
    bool isSupportedVideoFile(const QString& filePath) const;

    QString m_currentMediaPath;
};

#endif // FFMPEGMEDIADECODER_H
