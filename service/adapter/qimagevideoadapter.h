#ifndef QIMAGEVIDEOADAPTER_H
#define QIMAGEVIDEOADAPTER_H

#include "ivideoadapter.h"

#include <QImage>
#include <QObject>

class QImageVideoAdapter final : public QObject, public IVideoAdapter {
    Q_OBJECT

public:
    explicit QImageVideoAdapter(QObject* parent = nullptr);

    void onVideoFrame(const std::shared_ptr<VideoFrame>& frame) override;

signals:
    void frameAdapted(const QImage& frame);

private:
    QImage adapt(const VideoFrame& frame);
};

#endif // QIMAGEVIDEOADAPTER_H
