#ifndef FFMPEGSUBTITLEDECODER_H
#define FFMPEGSUBTITLEDECODER_H

#include <QObject>
#include <QString>

#include "pipelineworker.h"
#include "mediatypes.h"

class LogService;

class FFmpegSubtitleDecoder : public PipelineWorker
{
    Q_OBJECT

  public:
    explicit FFmpegSubtitleDecoder(QObject* parent = nullptr);
    ~FFmpegSubtitleDecoder() override;

  public slots:
    void configure(const WorkerConfigPtr& config) override;
    void closeMedia();
    void openMedia(const QString& filePath);
    void seek(qint64 positionMs);

  protected:
    void run() override;
    void flush() override;
    void release() override;
    void stop() override;

  signals:
    void subtitleFrameDecoded(const SubtitleFrame& frame);
};

#endif // FFMPEGSUBTITLEDECODER_H
