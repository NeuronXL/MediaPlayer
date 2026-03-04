#include "playbackcontrolwidget.h"

#include <QPalette>
#include <QtGlobal>

#include "ui_playbackcontrolwidget.h"

PlaybackControlWidget::PlaybackControlWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackControlWidget)
    , m_currentPositionMs(0)
    , m_durationMs(0)
    , m_playbackState(PlaybackState::Idle)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(28, 28, 28));
    setPalette(palette);

    ui->playButton->setCheckable(false);

    connect(ui->playButton, &QPushButton::clicked, this,
            &PlaybackControlWidget::handlePlayButtonClicked);

    updatePlayButtonAppearance();
    setCurrentPosition(0);
    setMediaInfo(MediaInfo{});
}

PlaybackControlWidget::~PlaybackControlWidget()
{
    delete ui;
}

void PlaybackControlWidget::setCurrentPosition(qint64 positionMs)
{
    m_currentPositionMs = qMax<qint64>(0, positionMs);
    ui->currentTimeLabel->setText(formatTime(m_currentPositionMs));

    const int sliderMaximum = qMax(1, ui->progressSlider->maximum());
    const int sliderValue =
        m_durationMs > 0
            ? static_cast<int>((m_currentPositionMs * sliderMaximum) / m_durationMs)
            : 0;
    ui->progressSlider->setValue(qBound(0, sliderValue, sliderMaximum));
}

void PlaybackControlWidget::setMediaInfo(const MediaInfo& mediaInfo)
{
    m_durationMs = qMax<qint64>(0, mediaInfo.durationMs);
    ui->durationLabel->setText(formatTime(m_durationMs));
    setCurrentPosition(m_currentPositionMs);
}

void PlaybackControlWidget::setPlaybackState(PlaybackState state)
{
    if (m_playbackState == state) {
        return;
    }

    m_playbackState = state;
    updatePlayButtonAppearance();
}

void PlaybackControlWidget::handlePlayButtonClicked()
{
    if (m_playbackState == PlaybackState::Playing) {
        emit pauseRequested();
        return;
    }

    emit playRequested();
}

QString PlaybackControlWidget::formatTime(qint64 positionMs) const
{
    const qint64 totalSeconds = qMax<qint64>(0, positionMs / 1000);
    const qint64 seconds = totalSeconds % 60;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 hours = totalSeconds / 3600;

    if (hours > 0) {
        return tr("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    return tr("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void PlaybackControlWidget::updatePlayButtonAppearance()
{
    ui->playButton->setIcon(QIcon());

    const bool isPlaying = (m_playbackState == PlaybackState::Playing);
    ui->playButton->setText(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setToolTip(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setStatusTip(isPlaying ? tr("Pause") : tr("Play"));
}
