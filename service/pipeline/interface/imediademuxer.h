#ifndef IMEDIADEMUXER_H
#define IMEDIADEMUXER_H

#include "../../common/packet.h"
#include "decoderinitdata.h"

#include <cstdint>
#include <memory>
#include <string>

class IMediaSource;
struct MediaSourceInfo;

class IDemuxPacketSink {
public:
    virtual ~IDemuxPacketSink() = default;

    virtual bool onDemuxPacket(const EncodedPacketPtr& packet) = 0;
    virtual void onDemuxEndOfStream() = 0;
    virtual void onDemuxError(const std::string& errorMessage) = 0;
};

class IMediaDemuxer {
public:
    virtual ~IMediaDemuxer() = default;

    virtual void setPacketSink(IDemuxPacketSink* sink) = 0;
    virtual bool open(const std::shared_ptr<IMediaSource>& source,
                      MediaSourceInfo* sourceInfo,
                      std::string* errorMessage) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool getVideoDecoderInitData(DecoderInitData* initData) const = 0;

    virtual void seek(int64_t positionMs) = 0;
    virtual void setAudioOutputFormat(int sampleRate, int channels, int sampleFormat) = 0;
};

#endif // IMEDIADEMUXER_H
