#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QImage>
#include <QObject>
#include <QWidget>

#include "../service/player/playbackstate.h"

namespace Ui
{
class VideoWidget;
}

class VideoWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit VideoWidget(QWidget* parent = nullptr);
    ~VideoWidget() override;

    void setFrame(const QImage& frame);
    void setPlaybackState(PlaybackState state);
    void clearFrame();

  protected:
    void paintEvent(QPaintEvent* event) override;

  private slots:
    void handlePlayButtonClicked();

  signals:
    void playRequested();
    void pauseRequested();

  private:
    void updatePlayButtonAppearance();

    Ui::VideoWidget* ui;
    QImage m_currentFrame;
    PlaybackState m_playbackState;
};

#endif // VIDEOWIDGET_H
