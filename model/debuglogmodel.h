#ifndef DEBUGLOGMODEL_H
#define DEBUGLOGMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>

class DebugLogModel : public QObject
{
    Q_OBJECT

  public:
    explicit DebugLogModel(QObject* parent = nullptr);
    ~DebugLogModel() override;

    QStringList logs() const;

  public slots:
    void appendLog(const QString& logMessage);

  signals:
    void logAdded(const QString& logMessage);

  private:
    QStringList m_logs;
};

#endif // DEBUGLOGMODEL_H
