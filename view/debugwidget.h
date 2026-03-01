#ifndef DEBUGWIDGET_H
#define DEBUGWIDGET_H

#include <QString>
#include <QStringList>
#include <QWidget>

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

    void setLogs(const QStringList& logs);

  public slots:
    void appendLog(const QString& logMessage);

  private:
    Ui::DebugWidget* ui;
};

#endif // DEBUGWIDGET_H
