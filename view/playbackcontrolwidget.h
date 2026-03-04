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

  private slots:
    void handlePlayButtonClicked();

  private:
    QString formatTime(qint64 positionMs) const;
    void updatePlayButtonAppearance();

    Ui::PlaybackControlWidget* ui;
    qint64 m_currentPositionMs;
    qint64 m_durationMs;
    PlaybackState m_playbackState;
};

#endif // PLAYBACKCONTROLWIDGET_H
