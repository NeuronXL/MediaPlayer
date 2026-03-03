#include "logmodel.h"

LogModel::LogModel(QObject* parent)
    : QObject(parent)
{
}

LogModel::~LogModel() = default;

LogEntries LogModel::logs() const
{
    return m_logs;
}

void LogModel::appendLog(const LogEntry& logEntry)
{
    if (logEntry.message.isEmpty()) {
        return;
    }

    m_logs.append(logEntry);
    emit logAdded(logEntry);
}
