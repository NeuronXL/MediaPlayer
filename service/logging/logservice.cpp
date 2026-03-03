#include "logservice.h"

#include <QDateTime>

#include "../../model/logentry.h"
#include "../../model/logmodel.h"

LogService::LogService(QObject* parent)
    : QObject(parent)
    , m_logModel(new LogModel(this))
{
}

LogService::~LogService() = default;

LogModel* LogService::model() const
{
    return m_logModel;
}

void LogService::append(const QString& message)
{
    if (message.isEmpty()) {
        return;
    }

    const LogEntry entry{QDateTime::currentDateTime(), message};
    m_logModel->appendLog(entry);
}

void LogService::append(const LogEntry& entry)
{
    m_logModel->appendLog(entry);
}
