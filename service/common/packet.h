#ifndef PACKET_H
#define PACKET_H
#include <memory>
#include <stdint.h>

enum class TrackType { Video, Audio, Subtitle };

enum class CodecId {
    Unknown = 0,
    FFmpegNative
};

enum PacketFlags : uint32_t {
    PacketFlagNone = 0,
    PacketFlagKey = 1u << 0,
    PacketFlagCorrupt = 1u << 1,
    PacketFlagDiscard = 1u << 2
};

struct EncodedPacket {
public:
    TrackType track = TrackType::Video;
    int streamIndex = -1;
    int serial = 0;

    int64_t ptsMs = -1;
    int64_t dtsMs = -1;
    int64_t durationMs = 0;
    int64_t posBytes = -1;

    uint32_t flags = PacketFlagNone;
    CodecId codecId = CodecId::Unknown;

    const uint8_t* data = nullptr;
    int size = 0;

    // Hold packet memory/resource lifetime for zero-copy style transport.
    std::shared_ptr<void> payloadOwner;
    // Optional backend-specific handle (for FFmpeg path, this is AVPacket* holder).
    std::shared_ptr<void> nativeHandle;
};

using EncodedPacketPtr = std::shared_ptr<EncodedPacket>;

#endif // PACKET_H
