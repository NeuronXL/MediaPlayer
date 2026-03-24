#include "playbackcontrolwidget.h"

#include <QPalette>
#include <QtGlobal>

#include "ui_playbackcontrolwidget.h"

PlaybackControlWidget::PlaybackControlWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackControlWidget)
    , m_currentPositionMs(0)
    , m_durationMs(0)
    , m_isSeeking(false)
    , m_playbackState(PlayState::Idle)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(28, 28, 28));
    setPalette(palette);

    ui->playButton->setCheckable(false);

    connect(ui->playButton, &QPushButton::clicked, this, &PlaybackControlWidget::handlePlayButtonClicked);
    connect(ui->progressSlider, &QSlider::sliderMoved, this, &PlaybackControlWidget::handleProgressSliderMoved);
    connect(ui->progressSlider, &QSlider::sliderPressed, this, &PlaybackControlWidget::handleProgressSliderPressed);
    connect(ui->progressSlider, &QSlider::sliderReleased, this, &PlaybackControlWidget::handleProgressSliderReleased);

    updatePlayButtonAppearance();
    setDuration(0);
    setCurrentPosition(0);
}

PlaybackControlWidget::~PlaybackControlWidget()
{
    delete ui;
}

void PlaybackControlWidget::setCurrentPosition(qint64 positionMs)
{
    m_currentPositionMs = qMax<qint64>(0, positionMs);
    if (m_isSeeking) {
        return;
    }

    ui->currentTimeLabel->setText(formatTime(m_currentPositionMs));

    const int sliderMaximum = qMax(1, ui->progressSlider->maximum());
    const int sliderValue = m_durationMs > 0 ? static_cast<int>((m_currentPositionMs * sliderMaximum) / m_durationMs) : 0;
    ui->progressSlider->setValue(qBound(0, sliderValue, sliderMaximum));
}

void PlaybackControlWidget::setDuration(qint64 durationMs)
{
    m_durationMs = qMax<qint64>(0, durationMs);
    ui->durationLabel->setText(formatTime(m_durationMs));

    if (m_durationMs == 0) {
        ui->progressSlider->setValue(0);
        return;
    }

    const int sliderMaximum = qMax(1, ui->progressSlider->maximum());
    const int sliderValue = static_cast<int>((m_currentPositionMs * sliderMaximum) / m_durationMs);
    ui->progressSlider->setValue(qBound(0, sliderValue, sliderMaximum));
}

void PlaybackControlWidget::setPlayState(PlayState state)
{
    if (m_playbackState == state) {
        return;
    }

    m_playbackState = state;
    updatePlayButtonAppearance();
}

void PlaybackControlWidget::handlePlayButtonClicked()
{
    if (m_playbackState == PlayState::Playing) {
        emit pauseRequested();
        return;
    }

    emit playRequested();
}

void PlaybackControlWidget::handleProgressSliderMoved(int value)
{
    const qint64 positionMs = positionFromSliderValue(value);
    ui->currentTimeLabel->setText(formatTime(positionMs));
}

void PlaybackControlWidget::handleProgressSliderPressed()
{
    m_isSeeking = true;
    handleProgressSliderMoved(ui->progressSlider->value());
}

void PlaybackControlWidget::handleProgressSliderReleased()
{
    const qint64 positionMs = positionFromSliderValue(ui->progressSlider->value());
    m_currentPositionMs = positionMs;
    m_isSeeking = false;
    ui->currentTimeLabel->setText(formatTime(positionMs));
    emit seekRequested(positionMs);
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

qint64 PlaybackControlWidget::positionFromSliderValue(int value) const
{
    const int sliderMaximum = qMax(1, ui->progressSlider->maximum());
    if (m_durationMs <= 0) {
        return 0;
    }

    return (static_cast<qint64>(qBound(0, value, sliderMaximum)) * m_durationMs) /
           sliderMaximum;
}

void PlaybackControlWidget::updatePlayButtonAppearance()
{
    ui->playButton->setIcon(QIcon());

    const bool isPlaying = (m_playbackState == PlayState::Playing);
    ui->playButton->setText(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setToolTip(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setStatusTip(isPlaying ? tr("Pause") : tr("Play"));
}
