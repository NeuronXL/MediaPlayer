#include "mainwindow.h"

#include <QFileDialog>
#include <QPropertyAnimation>
#include <QStringList>
#include <QVBoxLayout>

#include "logwidget.h"
#include "playbackcontrolwidget.h"
#include "videowidget.h"
#include "ui_mainwindow.h"
#include "../viewmodel/mainwindowviewmodel.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_logWidget(nullptr)
    , m_debugPanelAnimation(nullptr)
    , m_playbackControlWidget(nullptr)
    , m_videoWidget(nullptr)
    , m_viewModel(new MainWindowViewModel(this))
    , m_debugPanelExpandedHeight(240)
    , m_windowBaseHeight(0)
{
    ui->setupUi(this);
    ui->menubar->setNativeMenuBar(false);
    ui->debugSeparator->setFixedHeight(1);
    ui->debugPanelContainer->hide();

    auto* contentLayout = new QVBoxLayout(ui->contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto* videoPanelWidget = new QWidget(ui->contentWidget);
    videoPanelWidget->setObjectName("videoPanelWidget");
    auto* videoPanelLayout = new QVBoxLayout(videoPanelWidget);
    videoPanelLayout->setContentsMargins(0, 0, 0, 0);
    videoPanelLayout->setSpacing(0);

    m_videoWidget = new VideoWidget(videoPanelWidget);
    m_playbackControlWidget = new PlaybackControlWidget(videoPanelWidget);

    videoPanelLayout->addWidget(m_videoWidget, 1);
    videoPanelLayout->addWidget(m_playbackControlWidget, 0);
    contentLayout->addWidget(videoPanelWidget);

    auto* debugLayout = new QVBoxLayout(ui->debugPanelContainer);
    debugLayout->setContentsMargins(0, 0, 0, 0);
    debugLayout->setSpacing(0);

    m_logWidget = new LogWidget(ui->debugPanelContainer);
    debugLayout->addWidget(m_logWidget);

    m_debugPanelExpandedHeight = qMax(180, m_logWidget->sizeHint().height());

    m_debugPanelAnimation = new QPropertyAnimation(ui->debugPanelContainer, "maximumHeight", this);
    m_debugPanelAnimation->setDuration(220);
    m_debugPanelAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_debugPanelAnimation, &QPropertyAnimation::valueChanged, this, &MainWindow::handleDebugPanelAnimationValueChanged);
    connect(m_debugPanelAnimation, &QPropertyAnimation::finished, this, &MainWindow::handleDebugPanelAnimationFinished);

    ui->actionDebug->setCheckable(true);

    connect(ui->actionOpen, &QAction::triggered, m_viewModel, &MainWindowViewModel::requestOpenFile);
    connect(ui->actionDebug, &QAction::triggered, this, &MainWindow::toggleDebugPanel);
    connect(m_viewModel, &MainWindowViewModel::openFileRequested, this, &MainWindow::openFileDialog);
    connect(m_playbackControlWidget, &PlaybackControlWidget::playRequested, m_viewModel, &MainWindowViewModel::play);
    connect(m_viewModel, &MainWindowViewModel::frameReady, m_videoWidget, &VideoWidget::setFrame);
    connect(m_viewModel, &MainWindowViewModel::logEntryAdded, m_logWidget, &LogWidget::appendLog);
}

void MainWindow::handleMediaInfoChanged(const MediaInfo& mediaInfo)
{
    
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::openFileDialog()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open Video File"), QString(),
        tr("Video Files (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm *.m4v "
           "*.ts *.mts *.m2ts *.mpeg *.mpg *.3gp *.ogv)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_viewModel->setSelectedFilePath(filePath);
}

void MainWindow::handleDebugPanelAnimationFinished()
{
    if (ui->debugPanelContainer->maximumHeight() == 0) {
        ui->debugPanelContainer->hide();
        return;
    }

    ui->debugPanelContainer->setMinimumHeight(m_debugPanelExpandedHeight);
    ui->debugPanelContainer->setMaximumHeight(m_debugPanelExpandedHeight);
}

void MainWindow::handleDebugPanelAnimationValueChanged(const QVariant& value)
{
    const int panelHeight = value.toInt();
    ui->debugPanelContainer->setMinimumHeight(panelHeight);

    if (!isFullScreen()) {
        resize(width(), m_windowBaseHeight + panelHeight);
    }
}

void MainWindow::toggleDebugPanel()
{
    animateDebugPanel(ui->actionDebug->isChecked());
}

void MainWindow::animateDebugPanel(bool expand)
{
    m_debugPanelAnimation->stop();

    const int currentHeight = ui->debugPanelContainer->maximumHeight();
    const int targetHeight = expand ? m_debugPanelExpandedHeight : 0;
    m_windowBaseHeight = height() - currentHeight;

    if (expand) {
        ui->debugPanelContainer->show();
    }

    m_debugPanelAnimation->setStartValue(currentHeight);
    m_debugPanelAnimation->setEndValue(targetHeight);
    m_debugPanelAnimation->start();
}

void MainWindow::updateSelectedFilePath(const QString& filePath)
{
    Q_UNUSED(filePath);
}
