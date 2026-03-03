#include "playbackcontrolwidget.h"

#include <QPalette>

#include "ui_playbackcontrolwidget.h"

PlaybackControlWidget::PlaybackControlWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackControlWidget)
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
}

PlaybackControlWidget::~PlaybackControlWidget()
{
    delete ui;
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

void PlaybackControlWidget::updatePlayButtonAppearance()
{
    ui->playButton->setIcon(QIcon());

    const bool isPlaying = (m_playbackState == PlaybackState::Playing);
    ui->playButton->setText(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setToolTip(isPlaying ? tr("Pause") : tr("Play"));
    ui->playButton->setStatusTip(isPlaying ? tr("Pause") : tr("Play"));
}
