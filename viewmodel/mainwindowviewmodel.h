#ifndef MAINWINDOWVIEWMODEL_H
#define MAINWINDOWVIEWMODEL_H

#include <QImage>
#include <QObject>
#include <QString>

#include "../service/player/mediainfo.h"
#include "../service/player/playbackstate.h"

class LogModel;
class LogService;
class MediaPlayerEngine;

class MainWindowViewModel : public QObject
{
    Q_OBJECT

  public:
    explicit MainWindowViewModel(QObject* parent = nullptr);
    ~MainWindowViewModel() override;

    LogModel* logModel() const;
    MediaInfo mediaInfo() const;
    PlaybackState playbackState() const;
    qint64 currentPositionMs() const;
    QString selectedFilePath() const;

  public slots:
    void appendLog(const QString& logMessage);
    void pausePlayback();
    void playPlayback();
    void requestOpenFile();
    void setSelectedFilePath(const QString& filePath);

  private slots:
    void handleCurrentMediaPathChanged(const QString& filePath);
    void handleMediaOpened(const QString& filePath);
    void handleMediaOpenFailed(const QString& filePath, const QString& reason);
    void handleMediaInfoChanged(const MediaInfo& mediaInfo);
    void handleMediaOpenStarted(const QString& filePath);
    void handleCurrentPositionChanged(qint64 positionMs);
    void handlePlaybackStateChanged(PlaybackState state);

  signals:
    void mediaInfoChanged(const MediaInfo& mediaInfo);
    void openFileRequested();
    void currentPositionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void playbackStateChanged(PlaybackState state);
    void previewFrameChanged(const QImage& frame);
    void selectedFilePathChanged(const QString& filePath);

  private:
    LogService* m_logService;
    MediaPlayerEngine* m_mediaPlayerEngine;
    MediaInfo m_mediaInfo;
    qint64 m_currentPositionMs;
    PlaybackState m_playbackState;
    QString m_selectedFilePath;
};

#endif // MAINWINDOWVIEWMODEL_H
