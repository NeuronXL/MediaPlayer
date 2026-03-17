#ifndef LOGSERVICE_H
#define LOGSERVICE_H

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../../model/logentry.h"
class LogModel;

class LogService {
public:
    using SubscriptionId = std::uint64_t;
    using LogCallback = std::function<void(const LogEntry&)>;

    static LogService& instance();

    LogService();
    ~LogService();

    LogModel& model();

    SubscriptionId subscribe(LogCallback callback);
    void unsubscribe(SubscriptionId subscriptionId);

    void append(const std::string& message);
    void append(const LogEntry& entry);

private:
    std::mutex m_mutex;
    SubscriptionId m_nextSubscriptionId;
    std::unordered_map<SubscriptionId, LogCallback> m_callbacks;
    LogModel* m_logModel;
};

#endif // LOGSERVICE_H
