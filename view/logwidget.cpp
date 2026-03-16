#include "logwidget.h"

#include <QPlainTextEdit>
#include "ui_logwidget.h"

LogWidget::LogWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LogWidget)
{
    ui->setupUi(this);
}

LogWidget::~LogWidget() { delete ui; }

void LogWidget::appendLog(const QString& message)
{
    if (message.isEmpty()) {
        return;
    }

    ui->logOutput->appendPlainText(message);
}

void LogWidget::clearLogs()
{
    ui->logOutput->clear();
}
