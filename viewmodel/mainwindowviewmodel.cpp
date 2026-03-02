#include "mainwindowviewmodel.h"

#include <QDateTime>

#include "../model/debuglogmodel.h"
#include "../service/player/mediaplayerengine.h"

MainWindowViewModel::MainWindowViewModel(QObject* parent)
    : QObject(parent)
    , m_debugLogModel(new DebugLogModel(this))
    , m_mediaPlayerEngine(new MediaPlayerEngine(this))
{
    connect(m_debugLogModel, &DebugLogModel::logAdded, this,
            &MainWindowViewModel::debugLogAdded);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpenStarted, this,
            [this](const QString& filePath) {
                appendDebugLog(tr("Opening media file: %1").arg(filePath));
            });
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::debugMessageGenerated, this,
            &MainWindowViewModel::appendDebugLog);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpened, this,
            [this](const QString& filePath) {
                if (m_selectedFilePath != filePath) {
                    m_selectedFilePath = filePath;
                    emit selectedFilePathChanged(m_selectedFilePath);
                }

                appendDebugLog(tr("Media file loaded: %1").arg(filePath));
            });
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpenFailed, this,
            [this](const QString& filePath, const QString& reason) {
                appendDebugLog(
                    tr("Failed to load media file: %1 (%2)").arg(filePath,
                                                                  reason));
            });
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::currentMediaPathChanged,
            this, [this](const QString& filePath) {
                if (m_selectedFilePath == filePath) {
                    return;
                }

                m_selectedFilePath = filePath;
                emit selectedFilePathChanged(m_selectedFilePath);
            });
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::firstFrameReady, this,
            &MainWindowViewModel::previewFrameChanged);
}

MainWindowViewModel::~MainWindowViewModel() = default;

QString MainWindowViewModel::selectedFilePath() const
{
    return m_selectedFilePath;
}

DebugLogEntries MainWindowViewModel::debugLogs() const
{
    return m_debugLogModel->logs();
}

void MainWindowViewModel::appendDebugLog(const QString& logMessage)
{
    if (logMessage.isEmpty()) {
        return;
    }

    const DebugLogEntry logEntry{QDateTime::currentDateTime(), logMessage};
    m_debugLogModel->appendLog(logEntry);
}

void MainWindowViewModel::requestOpenFile()
{
    emit openFileRequested();
}

void MainWindowViewModel::setSelectedFilePath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    m_mediaPlayerEngine->openMedia(filePath);
}
