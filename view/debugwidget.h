#ifndef DEBUGWIDGET_H
#define DEBUGWIDGET_H

#include <QWidget>

#include "../model/debuglogentry.h"

namespace Ui
{
class DebugWidget;
}

class DebugWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit DebugWidget(QWidget* parent = nullptr);
    ~DebugWidget();

    void setLogs(const DebugLogEntries& logs);

  public slots:
    void appendLog(const DebugLogEntry& logEntry);

  private:
    Ui::DebugWidget* ui;
};

#endif // DEBUGWIDGET_H
