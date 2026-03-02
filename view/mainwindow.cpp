#include "mainwindow.h"

#include <QFileDialog>
#include <QPropertyAnimation>
#include <QVBoxLayout>

#include "debugwidget.h"
#include "videowidget.h"
#include "ui_mainwindow.h"
#include "../viewmodel/mainwindowviewmodel.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_debugWidget(nullptr)
    , m_debugPanelAnimation(nullptr)
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

    m_videoWidget = new VideoWidget(ui->contentWidget);
    contentLayout->addWidget(m_videoWidget);

    auto* debugLayout = new QVBoxLayout(ui->debugPanelContainer);
    debugLayout->setContentsMargins(0, 0, 0, 0);
    debugLayout->setSpacing(0);

    m_debugWidget = new DebugWidget(ui->debugPanelContainer);
    debugLayout->addWidget(m_debugWidget);
    m_debugWidget->setLogs(m_viewModel->debugLogs());

    m_debugPanelExpandedHeight = qMax(180, m_debugWidget->sizeHint().height());

    m_debugPanelAnimation =
        new QPropertyAnimation(ui->debugPanelContainer, "maximumHeight", this);
    m_debugPanelAnimation->setDuration(220);
    m_debugPanelAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_debugPanelAnimation, &QPropertyAnimation::valueChanged, this,
            [this](const QVariant& value) {
                const int panelHeight = value.toInt();
                ui->debugPanelContainer->setMinimumHeight(panelHeight);

                if (!isFullScreen()) {
                    resize(width(), m_windowBaseHeight + panelHeight);
                }
            });

    connect(m_debugPanelAnimation, &QPropertyAnimation::finished, this, [this] {
        if (ui->debugPanelContainer->maximumHeight() == 0) {
            ui->debugPanelContainer->hide();
        } else {
            ui->debugPanelContainer->setMinimumHeight(
                m_debugPanelExpandedHeight);
            ui->debugPanelContainer->setMaximumHeight(
                m_debugPanelExpandedHeight);
        }
    });

    ui->actionDebug->setCheckable(true);

    connect(ui->actionOpen, &QAction::triggered, m_viewModel,
            &MainWindowViewModel::requestOpenFile);
    connect(ui->actionDebug, &QAction::triggered, this,
            &MainWindow::toggleDebugPanel);
    connect(m_viewModel, &MainWindowViewModel::openFileRequested, this,
            &MainWindow::openFileDialog);
    connect(m_viewModel, &MainWindowViewModel::debugLogAdded, m_debugWidget,
            &DebugWidget::appendLog);
    connect(m_viewModel, &MainWindowViewModel::previewFrameChanged,
            m_videoWidget, &VideoWidget::setFrame);
    connect(m_viewModel, &MainWindowViewModel::selectedFilePathChanged, this,
            &MainWindow::updateSelectedFilePath);
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
    m_viewModel->appendDebugLog(tr("Selected file: %1").arg(filePath));
    Q_UNUSED(filePath);
}
