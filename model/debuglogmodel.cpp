#include "debuglogmodel.h"

DebugLogModel::DebugLogModel(QObject* parent)
    : QObject(parent)
{
}

DebugLogModel::~DebugLogModel() = default;

QStringList DebugLogModel::logs() const
{
    return m_logs;
}

void DebugLogModel::appendLog(const QString& logMessage)
{
    if (logMessage.isEmpty()) {
        return;
    }

    m_logs.append(logMessage);
    emit logAdded(logMessage);
}
