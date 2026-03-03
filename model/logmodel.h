#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QObject>

#include "logentry.h"

class LogModel : public QObject
{
    Q_OBJECT

  public:
    explicit LogModel(QObject* parent = nullptr);
    ~LogModel() override;

    LogEntries logs() const;

  public slots:
    void appendLog(const LogEntry& logEntry);

  signals:
    void logAdded(const LogEntry& logEntry);

  private:
    LogEntries m_logs;
};

#endif // LOGMODEL_H
