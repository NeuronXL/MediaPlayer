#ifndef MEDIAPLAYERENGINE_H
#define MEDIAPLAYERENGINE_H

#include <QObject>
#include <QString>

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
    void mediaOpenStarted(const QString& filePath);
    void mediaOpened(const QString& filePath);
    void mediaOpenFailed(const QString& filePath, const QString& reason);
    void currentMediaPathChanged(const QString& filePath);

  private:
    FFmpegMediaDecoder* m_mediaDecoder;
};

#endif // MEDIAPLAYERENGINE_H
