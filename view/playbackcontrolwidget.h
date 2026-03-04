#ifndef PLAYBACKCONTROLWIDGET_H
#define PLAYBACKCONTROLWIDGET_H

#include <QWidget>

#include "../service/player/mediainfo.h"
#include "../service/player/playbackstate.h"

namespace Ui
{
class PlaybackControlWidget;
}

class PlaybackControlWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit PlaybackControlWidget(QWidget* parent = nullptr);
    ~PlaybackControlWidget() override;

    void setCurrentPosition(qint64 positionMs);
    void setMediaInfo(const MediaInfo& mediaInfo);
    void setPlaybackState(PlaybackState state);

  signals:
    void playRequested();
    void pauseRequested();
    void seekRequested(qint64 positionMs);

  private slots:
    void handlePlayButtonClicked();
    void handleProgressSliderMoved(int value);
    void handleProgressSliderPressed();
    void handleProgressSliderReleased();

  private:
    QString formatTime(qint64 positionMs) const;
    qint64 positionFromSliderValue(int value) const;
    void updatePlayButtonAppearance();

    Ui::PlaybackControlWidget* ui;
    qint64 m_currentPositionMs;
    qint64 m_durationMs;
    bool m_isSeeking;
    PlaybackState m_playbackState;
};

#endif // PLAYBACKCONTROLWIDGET_H
