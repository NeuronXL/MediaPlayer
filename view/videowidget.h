#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QImage>
#include <QObject>
#include <QWidget>

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
    void clearFrame();

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    Ui::VideoWidget* ui;
    QImage m_currentFrame;
};

#endif // VIDEOWIDGET_H
