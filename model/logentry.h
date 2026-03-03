#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

struct LogEntry
{
    QDateTime timestamp;
    QString message;
};

using LogEntries = QList<LogEntry>;

inline QString formatLogEntry(const LogEntry& entry)
{
    const QString timestampText = entry.timestamp.isValid()
        ? entry.timestamp.toString("yyyy-MM-dd HH:mm:ss")
        : QStringLiteral("0000-00-00 00:00:00");

    return QString("[%1] %2").arg(timestampText, entry.message);
}

Q_DECLARE_METATYPE(LogEntry)

#endif // LOGENTRY_H
