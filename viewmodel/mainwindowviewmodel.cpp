#include "mainwindowviewmodel.h"

#include "../model/logmodel.h"
#include "../service/logging/logservice.h"
#include "../service/player/mediaplayerengine.h"

MainWindowViewModel::MainWindowViewModel(QObject* parent)
    : QObject(parent)
    , m_logService(new LogService(this))
    , m_mediaPlayerEngine(new MediaPlayerEngine(m_logService, this))
{
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpenStarted, this,
            [this](const QString& filePath) {
                appendLog(tr("Opening media file: %1").arg(filePath));
            });
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpened, this,
            [this](const QString& filePath) {
                if (m_selectedFilePath != filePath) {
                    m_selectedFilePath = filePath;
                    emit selectedFilePathChanged(m_selectedFilePath);
                }

                appendLog(tr("Media file loaded: %1").arg(filePath));
            });
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpenFailed, this,
            [this](const QString& filePath, const QString& reason) {
                appendLog(
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

LogModel* MainWindowViewModel::logModel() const
{
    return m_logService->model();
}

QString MainWindowViewModel::selectedFilePath() const
{
    return m_selectedFilePath;
}

void MainWindowViewModel::appendLog(const QString& logMessage)
{
    m_logService->append(logMessage);
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
