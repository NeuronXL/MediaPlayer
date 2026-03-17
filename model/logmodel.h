#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <mutex>

#include "logentry.h"

class LogModel {
public:
    LogModel() = default;
    ~LogModel() = default;

    LogEntries logs() const;

    void appendLog(const LogEntry& logEntry);

private:
    mutable std::mutex m_mutex;
    LogEntries m_logs;
};

#endif // LOGMODEL_H
