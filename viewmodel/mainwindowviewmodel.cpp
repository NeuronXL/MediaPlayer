#include "mainwindowviewmodel.h"

#include "../model/debuglogmodel.h"

MainWindowViewModel::MainWindowViewModel(QObject* parent)
    : QObject(parent)
    , m_debugLogModel(new DebugLogModel(this))
{
    connect(m_debugLogModel, &DebugLogModel::logAdded, this,
            &MainWindowViewModel::debugLogAdded);
}

MainWindowViewModel::~MainWindowViewModel() = default;

QString MainWindowViewModel::selectedFilePath() const
{
    return m_selectedFilePath;
}

QStringList MainWindowViewModel::debugLogs() const
{
    return m_debugLogModel->logs();
}

void MainWindowViewModel::appendDebugLog(const QString& logMessage)
{
    m_debugLogModel->appendLog(logMessage);
}

void MainWindowViewModel::requestOpenFile()
{
    emit openFileRequested();
}

void MainWindowViewModel::setSelectedFilePath(const QString& filePath)
{
    if (m_selectedFilePath == filePath) {
        return;
    }

    m_selectedFilePath = filePath;
    emit selectedFilePathChanged(m_selectedFilePath);
}
