#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <chrono>
#include <string>
#include <vector>

struct LogEntry
{
    std::chrono::system_clock::time_point timestamp;
    std::string message;
};

using LogEntries = std::vector<LogEntry>;

std::string formatLogEntry(const LogEntry& entry);

#endif // LOGENTRY_H
