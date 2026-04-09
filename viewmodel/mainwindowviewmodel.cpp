#include "mainwindowviewmodel.h"

#include <QMetaObject>
#include <QPointer>
#include <memory>

#include "../service/adapter/qimagevideoadapter.h"
#include "../service/adapter/qaudiooutputadapter.h"
#include "../model/logentry.h"
#include "../service/logging/logservice.h"
#include "../service/player/mediaplayerengine.h"

MainWindowViewModel::MainWindowViewModel(QObject* parent)
    : QObject(parent)
    , m_mediaPlayerEngine(nullptr)
    , m_engineSubscriptionId(0)
    , m_logSubscriptionId(0)
{
    m_videoAdapter = std::make_shared<QImageVideoAdapter>();
    m_audioAdapter = std::make_shared<QAudioOutputAdapter>();
    m_mediaPlayerEngine = new MediaPlayerEngine(m_videoAdapter, m_audioAdapter);

    connect(m_videoAdapter.get(), &QImageVideoAdapter::frameAdapted, this, &MainWindowViewModel::frameReady);

    QPointer<MainWindowViewModel> eventGuard(this);
    m_engineSubscriptionId = m_mediaPlayerEngine->subscribe(
        ENGINE_EVENT_OPEN_MEDIA_SUCCEEDED | ENGINE_EVENT_OPEN_MEDIA_FAILED,
        [eventGuard](const EngineEvent& event) {
            if (!eventGuard) {
                return;
            }
            QMetaObject::invokeMethod(eventGuard, [eventGuard, event]() {
                if (!eventGuard) {
                    return;
                }

                if (const auto* openEvent = std::get_if<OpenMediaSucceededEvent>(&event)) {
                    MediaInfo mediaInfo;
                    mediaInfo.filePath = QString::fromStdString(openEvent->mediaInfo.filePath);
                    mediaInfo.containerFormat = QString::fromStdString(openEvent->mediaInfo.containerFormat);
                    mediaInfo.videoCodec = QString::fromStdString(openEvent->mediaInfo.videoCodec);
                    mediaInfo.durationMs = openEvent->mediaInfo.durationMs;
                    mediaInfo.width = openEvent->mediaInfo.width;
                    mediaInfo.height = openEvent->mediaInfo.height;
                    mediaInfo.frameRate = openEvent->mediaInfo.frameRate;
                    emit eventGuard->mediaInfoChanged(mediaInfo);
                    return;
                }

                if (const auto* failedEvent = std::get_if<OpenMediaFailedEvent>(&event)) {
                    emit eventGuard->logEntryAdded(QString::fromStdString(
                        "open media failed: " + failedEvent->filePath + " reason=" + failedEvent->errorMessage));
                }
            }, Qt::QueuedConnection);
        });

    QPointer<MainWindowViewModel> guard(this);
    m_logSubscriptionId = LogService::instance().subscribe([guard](const LogEntry& entry) {
        if (!guard) {
            return;
        }

        const QString message = QString::fromStdString(formatLogEntry(entry));
        QMetaObject::invokeMethod(guard, [guard, message]() {
            if (guard) {
                emit guard->logEntryAdded(message);
            }
        }, Qt::QueuedConnection);
    });
}

MainWindowViewModel::~MainWindowViewModel() {
    if (m_engineSubscriptionId != 0) {
        m_mediaPlayerEngine->unsubscribe(m_engineSubscriptionId);
        m_engineSubscriptionId = 0;
    }
    if (m_logSubscriptionId != 0) {
        LogService::instance().unsubscribe(m_logSubscriptionId);
        m_logSubscriptionId = 0;
    }
    delete m_mediaPlayerEngine;
    m_mediaPlayerEngine = nullptr;
}

void MainWindowViewModel::play()
{
    m_mediaPlayerEngine->play();
    emit playStateChanged(PlayState::Playing);
}

void MainWindowViewModel::pause()
{
    m_mediaPlayerEngine->pause();
    emit playStateChanged(PlayState::Paused);
}

void MainWindowViewModel::seek(qint64 positionMs)
{
    m_mediaPlayerEngine->seek(positionMs);
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

    m_mediaPlayerEngine->openMedia(filePath.toStdString());
}
