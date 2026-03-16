#include "logmodel.h"

LogEntries LogModel::logs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs;
}

void LogModel::appendLog(const LogEntry& logEntry)
{
    if (logEntry.message.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back(logEntry);
}
