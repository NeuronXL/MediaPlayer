#ifndef DEBUGLOGENTRY_H
#define DEBUGLOGENTRY_H

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

struct DebugLogEntry
{
    QDateTime timestamp;
    QString message;
};

using DebugLogEntries = QList<DebugLogEntry>;

inline QString formatDebugLogEntry(const DebugLogEntry& entry)
{
    const QString timestampText = entry.timestamp.isValid()
        ? entry.timestamp.toString("yyyy-MM-dd HH:mm:ss")
        : QStringLiteral("0000-00-00 00:00:00");

    return QString("[%1] %2").arg(timestampText, entry.message);
}

Q_DECLARE_METATYPE(DebugLogEntry)

#endif // DEBUGLOGENTRY_H
