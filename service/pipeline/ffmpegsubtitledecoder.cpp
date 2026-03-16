#include "ffmpegsubtitledecoder.h"

#include "../logging/logservice.h"

FFmpegSubtitleDecoder::FFmpegSubtitleDecoder(QObject* parent)
    : PipelineWorker(parent)
{
}

FFmpegSubtitleDecoder::~FFmpegSubtitleDecoder() = default;

void FFmpegSubtitleDecoder::configure(const WorkerConfigPtr& config)
{
    Q_UNUSED(config);
}

void FFmpegSubtitleDecoder::flush()
{
}

void FFmpegSubtitleDecoder::run()
{
}

void FFmpegSubtitleDecoder::release()
{
}

void FFmpegSubtitleDecoder::stop()
{
}

void FFmpegSubtitleDecoder::openMedia(const QString& filePath)
{
}

void FFmpegSubtitleDecoder::closeMedia()
{
}

void FFmpegSubtitleDecoder::seek(qint64 positionMs)
{
    Q_UNUSED(positionMs);
}
