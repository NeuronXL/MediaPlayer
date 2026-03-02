#include "debuglogmodel.h"

DebugLogModel::DebugLogModel(QObject* parent)
    : QObject(parent)
{
}

DebugLogModel::~DebugLogModel() = default;

DebugLogEntries DebugLogModel::logs() const
{
    return m_logs;
}

void DebugLogModel::appendLog(const DebugLogEntry& logEntry)
{
    if (logEntry.message.isEmpty()) {
        return;
    }

    m_logs.append(logEntry);
    emit logAdded(logEntry);
}
