#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QMetaObject>
#include <QWidget>

#include "../model/logentry.h"

class LogModel;

namespace Ui
{
class LogWidget;
}

class LogWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit LogWidget(QWidget* parent = nullptr);
    ~LogWidget() override;

    void setLogModel(LogModel* logModel);

  public slots:
    void appendLog(const LogEntry& logEntry);

  private:
    void setLogs(const LogEntries& logs);

    Ui::LogWidget* ui;
    LogModel* m_logModel;
    QMetaObject::Connection m_logAddedConnection;
};

#endif // LOGWIDGET_H
