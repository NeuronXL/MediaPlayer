#ifndef DEBUGLOGMODEL_H
#define DEBUGLOGMODEL_H

#include <QObject>

#include "debuglogentry.h"

class DebugLogModel : public QObject
{
    Q_OBJECT

  public:
    explicit DebugLogModel(QObject* parent = nullptr);
    ~DebugLogModel() override;

    DebugLogEntries logs() const;

  public slots:
    void appendLog(const DebugLogEntry& logEntry);

  signals:
    void logAdded(const DebugLogEntry& logEntry);

  private:
    DebugLogEntries m_logs;
};

#endif // DEBUGLOGMODEL_H
