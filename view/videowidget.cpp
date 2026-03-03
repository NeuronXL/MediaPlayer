#include "videowidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QPalette>

#include "ui_videowidget.h"

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::VideoWidget)
    , m_playbackState(PlaybackState::Idle)
{
    ui->setupUi(this);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(18, 18, 18));
    setPalette(palette);

    ui->playButton->setCheckable(false);
    connect(ui->playButton, &QPushButton::clicked, this,
            &VideoWidget::handlePlayButtonClicked);
    updatePlayButtonAppearance();
}

VideoWidget::~VideoWidget()
{
    delete ui;
}

void VideoWidget::setFrame(const QImage& frame)
{
    m_currentFrame = frame;
    update();
}

void VideoWidget::setPlaybackState(PlaybackState state)
{
    if (m_playbackState == state) {
        return;
    }

    m_playbackState = state;
    updatePlayButtonAppearance();
}

void VideoWidget::clearFrame()
{
    m_currentFrame = QImage();
    update();
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_currentFrame.isNull()) {
        painter.setPen(QColor(160, 160, 160));
        painter.drawText(rect(), Qt::AlignCenter, tr("No video loaded"));
        return;
    }

    const QImage scaledFrame = m_currentFrame.scaled(
        size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const QPoint topLeft((width() - scaledFrame.width()) / 2,
                         (height() - scaledFrame.height()) / 2);
    painter.drawImage(topLeft, scaledFrame);
}

void VideoWidget::handlePlayButtonClicked()
{
    if (m_playbackState == PlaybackState::Playing) {
        emit pauseRequested();
        return;
    }

    emit playRequested();
}

void VideoWidget::updatePlayButtonAppearance()
{
    ui->playButton->setIcon(QIcon());
    const bool isPlaying = (m_playbackState == PlaybackState::Playing);
    ui->playButton->setText(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setToolTip(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setStatusTip(isPlaying ? tr("Pause") : tr("Play"));
}
