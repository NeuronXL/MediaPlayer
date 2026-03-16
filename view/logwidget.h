#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QString>
#include <QWidget>

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

  public slots:
    void appendLog(const QString& message);
    void clearLogs();

  private:
    Ui::LogWidget* ui;
};

#endif // LOGWIDGET_H
