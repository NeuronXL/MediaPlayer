#ifndef MAINWINDOWVIEWMODEL_H
#define MAINWINDOWVIEWMODEL_H

#include <QImage>
#include <QObject>
#include <QString>

#include <cstdint>
#include <memory>

#include "../service/player/mediainfo.h"

class MediaPlayerEngine;
class QImageVideoAdapter;
class QAudioOutputAdapter;

class MainWindowViewModel : public QObject
{
    Q_OBJECT

  public:
    explicit MainWindowViewModel(QObject* parent = nullptr);
    ~MainWindowViewModel() override;

  signals:
    void frameReady(const QImage& frame, qint64 playbackTimeMs);
    void mediaInfoChanged(const MediaInfo& mediaInfo);
    void logEntryAdded(const QString& message);
    void openFileRequested();

  public slots:
    void play();
    void requestOpenFile();
    void setSelectedFilePath(const QString& filePath);

  private:
    MediaPlayerEngine* m_mediaPlayerEngine;
    std::uint64_t m_engineSubscriptionId;
    std::uint64_t m_logSubscriptionId;
    std::shared_ptr<QImageVideoAdapter> m_videoAdapter;
    std::shared_ptr<QAudioOutputAdapter> m_audioAdapter;
    QString m_selectedFilePath;
};

#endif // MAINWINDOWVIEWMODEL_H
