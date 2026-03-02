#include "videowidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QPalette>

#include "ui_videowidget.h"

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::VideoWidget)
    , m_isPlaying(false)
{
    ui->setupUi(this);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(18, 18, 18));
    setPalette(palette);

    connect(ui->playButton, &QPushButton::toggled, this,
            &VideoWidget::togglePlaybackState);
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

void VideoWidget::togglePlaybackState(bool playing)
{
    m_isPlaying = playing;
    updatePlayButtonAppearance();
}

void VideoWidget::updatePlayButtonAppearance()
{
    ui->playButton->setIcon(QIcon());
    ui->playButton->setToolTip(m_isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setStatusTip(m_isPlaying ? tr("Pause") : tr("Play"));
}
