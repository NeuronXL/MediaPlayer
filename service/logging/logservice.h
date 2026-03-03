#ifndef LOGSERVICE_H
#define LOGSERVICE_H

#include <QObject>
#include <QString>

class LogModel;
struct LogEntry;

class LogService : public QObject
{
    Q_OBJECT

  public:
    explicit LogService(QObject* parent = nullptr);
    ~LogService() override;

    LogModel* model() const;

  public slots:
    void append(const QString& message);
    void append(const LogEntry& entry);

  private:
    LogModel* m_logModel;
};

#endif // LOGSERVICE_H
