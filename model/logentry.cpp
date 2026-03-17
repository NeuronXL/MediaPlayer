#include "logentry.h"

#include <ctime>
#include <iomanip>
#include <sstream>

std::string formatLogEntry(const LogEntry& entry)
{
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tmValue{};
#if defined(_WIN32)
    localtime_s(&tmValue, &timeValue);
#else
    localtime_r(&timeValue, &tmValue);
#endif

    std::ostringstream oss;
    oss << "[" << std::put_time(&tmValue, "%Y-%m-%d %H:%M:%S") << "] " << entry.message;
    return oss.str();
}

