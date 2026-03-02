#include "debugwidget.h"

#include <QPlainTextEdit>

#include "ui_debugwidget.h"

DebugWidget::DebugWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DebugWidget)
{
    ui->setupUi(this);
}

DebugWidget::~DebugWidget() { delete ui; }

void DebugWidget::setLogs(const DebugLogEntries& logs)
{
    QStringList formattedLogs;
    formattedLogs.reserve(logs.size());

    for (const DebugLogEntry& logEntry : logs) {
        formattedLogs.append(formatDebugLogEntry(logEntry));
    }

    ui->logOutput->setPlainText(formattedLogs.join('\n'));
}

void DebugWidget::appendLog(const DebugLogEntry& logEntry)
{
    if (logEntry.message.isEmpty()) {
        return;
    }

    ui->logOutput->appendPlainText(formatDebugLogEntry(logEntry));
}
