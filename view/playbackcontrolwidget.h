#ifndef PLAYBACKCONTROLWIDGET_H
#define PLAYBACKCONTROLWIDGET_H

#include <QWidget>

#include "../service/player/playstate.h"

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
    void setDuration(qint64 durationMs);
    void setPlayState(PlayState state);

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
    PlayState m_playbackState;
};

#endif // PLAYBACKCONTROLWIDGET_H
