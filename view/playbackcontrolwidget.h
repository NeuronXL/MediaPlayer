#ifndef PLAYBACKCONTROLWIDGET_H
#define PLAYBACKCONTROLWIDGET_H

#include <QWidget>

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

    void setPlaybackState(PlaybackState state);

  signals:
    void playRequested();
    void pauseRequested();

  private slots:
    void handlePlayButtonClicked();

  private:
    void updatePlayButtonAppearance();

    Ui::PlaybackControlWidget* ui;
    PlaybackState m_playbackState;
};

#endif // PLAYBACKCONTROLWIDGET_H
