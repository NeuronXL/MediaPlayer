#include "logservice.h"

#include <chrono>
#include <utility>
#include <vector>

#include "../../model/logentry.h"
#include "../../model/logmodel.h"

LogService& LogService::instance()
{
    static LogService s_instance;
    return s_instance;
}

LogService::LogService()
    : m_nextSubscriptionId(1)
    , m_logModel(new LogModel())
{
}

LogService::~LogService()
{
    delete m_logModel;
    m_logModel = nullptr;
}

LogModel& LogService::model()
{
    return *m_logModel;
}

LogService::SubscriptionId LogService::subscribe(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const SubscriptionId subscriptionId = m_nextSubscriptionId++;
    m_callbacks.emplace(subscriptionId, std::move(callback));
    return subscriptionId;
}

void LogService::unsubscribe(SubscriptionId subscriptionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.erase(subscriptionId);
}

void LogService::append(const std::string& message)
{
    if (message.empty()) {
        return;
    }

    append(LogEntry{std::chrono::system_clock::now(), message});
}

void LogService::append(const LogEntry& entry)
{
    m_logModel->appendLog(entry);

    std::vector<LogCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callbacks.reserve(m_callbacks.size());
        for (const auto& [id, callback] : m_callbacks) {
            (void)id;
            callbacks.push_back(callback);
        }
    }

    for (const auto& callback : callbacks) {
        if (callback) {
            callback(entry);
        }
    }
}
