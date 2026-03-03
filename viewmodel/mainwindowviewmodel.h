#ifndef MAINWINDOWVIEWMODEL_H
#define MAINWINDOWVIEWMODEL_H

#include <QImage>
#include <QObject>
#include <QString>

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
    PlaybackState playbackState() const;
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
    void handleMediaOpenStarted(const QString& filePath);
    void handlePlaybackStateChanged(PlaybackState state);

  signals:
    void openFileRequested();
    void playbackStateChanged(PlaybackState state);
    void previewFrameChanged(const QImage& frame);
    void selectedFilePathChanged(const QString& filePath);

  private:
    LogService* m_logService;
    MediaPlayerEngine* m_mediaPlayerEngine;
    PlaybackState m_playbackState;
    QString m_selectedFilePath;
};

#endif // MAINWINDOWVIEWMODEL_H
