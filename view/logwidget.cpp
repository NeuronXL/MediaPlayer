#include "logwidget.h"

#include <QPlainTextEdit>

#include "../model/logentry.h"
#include "../model/logmodel.h"
#include "ui_logwidget.h"

LogWidget::LogWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LogWidget)
    , m_logModel(nullptr)
{
    ui->setupUi(this);
}

LogWidget::~LogWidget() { delete ui; }

void LogWidget::setLogModel(LogModel* logModel)
{
    if (m_logModel == logModel) {
        return;
    }

    if (m_logAddedConnection) {
        disconnect(m_logAddedConnection);
    }

    m_logModel = logModel;
    ui->logOutput->clear();

    if (m_logModel == nullptr) {
        return;
    }

    setLogs(m_logModel->logs());
    m_logAddedConnection =
        connect(m_logModel, &LogModel::logAdded, this, &LogWidget::appendLog);
}

void LogWidget::appendLog(const LogEntry& logEntry)
{
    if (logEntry.message.isEmpty()) {
        return;
    }

    ui->logOutput->appendPlainText(formatLogEntry(logEntry));
}

void LogWidget::setLogs(const LogEntries& logs)
{
    QStringList formattedLogs;
    formattedLogs.reserve(logs.size());

    for (const LogEntry& logEntry : logs) {
        formattedLogs.append(formatLogEntry(logEntry));
    }

    ui->logOutput->setPlainText(formattedLogs.join('\n'));
}
