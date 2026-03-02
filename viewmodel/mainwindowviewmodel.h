#ifndef MAINWINDOWVIEWMODEL_H
#define MAINWINDOWVIEWMODEL_H

#include <QObject>
#include <QString>

#include "../model/debuglogentry.h"

class DebugLogModel;
class MediaPlayerEngine;

class MainWindowViewModel : public QObject
{
    Q_OBJECT

  public:
    explicit MainWindowViewModel(QObject* parent = nullptr);
    ~MainWindowViewModel() override;

    QString selectedFilePath() const;
    DebugLogEntries debugLogs() const;

  public slots:
    void appendDebugLog(const QString& logMessage);
    void requestOpenFile();
    void setSelectedFilePath(const QString& filePath);

  signals:
    void debugLogAdded(const DebugLogEntry& logEntry);
    void openFileRequested();
    void selectedFilePathChanged(const QString& filePath);

  private:
    DebugLogModel* m_debugLogModel;
    MediaPlayerEngine* m_mediaPlayerEngine;
    QString m_selectedFilePath;
};

#endif // MAINWINDOWVIEWMODEL_H
