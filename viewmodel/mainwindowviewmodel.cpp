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
    , m_logSubscriptionId(0)
{
    m_videoAdapter = std::make_shared<QImageVideoAdapter>();
    m_audioAdapter = std::make_shared<QAudioOutputAdapter>();
    m_mediaPlayerEngine = new MediaPlayerEngine(m_videoAdapter, m_audioAdapter);

    connect(m_videoAdapter.get(), &QImageVideoAdapter::frameAdapted, this, &MainWindowViewModel::frameReady);

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
