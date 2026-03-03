#ifndef MAINWINDOWVIEWMODEL_H
#define MAINWINDOWVIEWMODEL_H

#include <QImage>
#include <QObject>
#include <QString>

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
    QString selectedFilePath() const;

  public slots:
    void appendLog(const QString& logMessage);
    void requestOpenFile();
    void setSelectedFilePath(const QString& filePath);

  signals:
    void openFileRequested();
    void previewFrameChanged(const QImage& frame);
    void selectedFilePathChanged(const QString& filePath);

  private:
    LogService* m_logService;
    MediaPlayerEngine* m_mediaPlayerEngine;
    QString m_selectedFilePath;
};

#endif // MAINWINDOWVIEWMODEL_H
