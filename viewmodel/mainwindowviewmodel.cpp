#include "mainwindowviewmodel.h"

#include "../model/logmodel.h"
#include "../service/logging/logservice.h"
#include "../service/player/mediaplayerengine.h"

MainWindowViewModel::MainWindowViewModel(QObject* parent)
    : QObject(parent)
    , m_logService(new LogService(this))
    , m_mediaPlayerEngine(new MediaPlayerEngine(m_logService, this))
    , m_playbackState(PlaybackState::Idle)
{
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpenStarted, this,
            &MainWindowViewModel::handleMediaOpenStarted);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpened, this,
            &MainWindowViewModel::handleMediaOpened);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::mediaOpenFailed, this,
            &MainWindowViewModel::handleMediaOpenFailed);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::currentMediaPathChanged,
            this, &MainWindowViewModel::handleCurrentMediaPathChanged);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::firstFrameReady, this,
            &MainWindowViewModel::previewFrameChanged);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::frameReady, this,
            &MainWindowViewModel::previewFrameChanged);
    connect(m_mediaPlayerEngine, &MediaPlayerEngine::playbackStateChanged, this,
            &MainWindowViewModel::handlePlaybackStateChanged);
}

MainWindowViewModel::~MainWindowViewModel() = default;

LogModel* MainWindowViewModel::logModel() const
{
    return m_logService->model();
}

PlaybackState MainWindowViewModel::playbackState() const
{
    return m_playbackState;
}

QString MainWindowViewModel::selectedFilePath() const
{
    return m_selectedFilePath;
}

void MainWindowViewModel::appendLog(const QString& logMessage)
{
    m_logService->append(logMessage);
}

void MainWindowViewModel::pausePlayback()
{
    m_mediaPlayerEngine->pause();
}

void MainWindowViewModel::playPlayback()
{
    m_mediaPlayerEngine->play();
}

void MainWindowViewModel::requestOpenFile()
{
    emit openFileRequested();
}

void MainWindowViewModel::handleCurrentMediaPathChanged(const QString& filePath)
{
    if (m_selectedFilePath == filePath) {
        return;
    }

    m_selectedFilePath = filePath;
    emit selectedFilePathChanged(m_selectedFilePath);
}

void MainWindowViewModel::handleMediaOpened(const QString& filePath)
{
    if (m_selectedFilePath != filePath) {
        m_selectedFilePath = filePath;
        emit selectedFilePathChanged(m_selectedFilePath);
    }

    appendLog(tr("Media file loaded: %1").arg(filePath));
}

void MainWindowViewModel::handleMediaOpenFailed(const QString& filePath,
                                                const QString& reason)
{
    appendLog(tr("Failed to load media file: %1 (%2)").arg(filePath, reason));
}

void MainWindowViewModel::handleMediaOpenStarted(const QString& filePath)
{
    appendLog(tr("Opening media file: %1").arg(filePath));
}

void MainWindowViewModel::handlePlaybackStateChanged(PlaybackState state)
{
    if (m_playbackState == state) {
        return;
    }

    m_playbackState = state;
    emit playbackStateChanged(m_playbackState);
}

void MainWindowViewModel::setSelectedFilePath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    m_mediaPlayerEngine->openMedia(filePath);
}
