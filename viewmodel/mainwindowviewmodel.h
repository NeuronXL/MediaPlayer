#ifndef MAINWINDOWVIEWMODEL_H
#define MAINWINDOWVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>

class DebugLogModel;

class MainWindowViewModel : public QObject
{
    Q_OBJECT

  public:
    explicit MainWindowViewModel(QObject* parent = nullptr);
    ~MainWindowViewModel() override;

    QString selectedFilePath() const;
    QStringList debugLogs() const;

  public slots:
    void appendDebugLog(const QString& logMessage);
    void requestOpenFile();
    void setSelectedFilePath(const QString& filePath);

  signals:
    void debugLogAdded(const QString& logMessage);
    void openFileRequested();
    void selectedFilePathChanged(const QString& filePath);

  private:
    DebugLogModel* m_debugLogModel;
    QString m_selectedFilePath;
};

#endif // MAINWINDOWVIEWMODEL_H
