#include "debugwidget.h"

#include <QPlainTextEdit>

#include "ui_debugwidget.h"

DebugWidget::DebugWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DebugWidget)
{
    ui->setupUi(this);
}

DebugWidget::~DebugWidget() { delete ui; }

void DebugWidget::setLogs(const QStringList& logs)
{
    ui->logOutput->setPlainText(logs.join('\n'));
}

void DebugWidget::appendLog(const QString& logMessage)
{
    if (logMessage.isEmpty()) {
        return;
    }

    ui->logOutput->appendPlainText(logMessage);
}
