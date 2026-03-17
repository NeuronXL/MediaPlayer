#ifndef MEDIATYPES_H
#define MEDIATYPES_H

#include <QByteArray>
#include <QImage>
#include <QMetaType>
#include <QString>
#include <QtGlobal>

#include "../common/timestampeditem.h"
#include "workerconfig.h"

struct AVCodecParameters;
struct AVPacket;

enum class TrackType
{
    Video,
    Audio,
    Subtitle
};

struct MediaPacket : public TimestampedItem
{
    TrackType track = TrackType::Video;
    AVPacket* packet = nullptr;
};

struct AudioFormat
{
    int sampleRate = 0;
    int channels = 0;
    int sampleFormat = 0;
    qint64 channelLayout = 0;
};

struct VideoFrame
{
    qint64 ptsMs = 0;
    qint64 durationMs = 0;
    bool isKeyFrame = false;
    QImage image;
};

struct AudioFrame
{
    qint64 ptsMs = 0;
    qint64 durationMs = 0;
    QByteArray pcm;
    AudioFormat format;
};

struct SubtitleFrame
{
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;
};

struct VideoDecoderConfig : public WorkerConfig
{
    AVCodecParameters* codecParameters = nullptr;
    int timeBaseNum = 0;
    int timeBaseDen = 1;
    int frameIntervalMs = 33;
};

Q_DECLARE_OPAQUE_POINTER(AVPacket*)
Q_DECLARE_METATYPE(AVPacket*)
Q_DECLARE_METATYPE(VideoDecoderConfig)

#endif // MEDIATYPES_H
