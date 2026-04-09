#ifndef DECODERINITDATA_H
#define DECODERINITDATA_H

#include "../../common/packet.h"

#include <cstdint>
#include <vector>

struct DecoderInitData {
    TrackType trackType = TrackType::Video;
    int streamIndex = -1;
    CodecId codecId = CodecId::Unknown;
    int nativeCodecId = -1;

    int timeBaseNum = 0;
    int timeBaseDen = 1;
    int64_t startTimeMs = -1;
    int64_t durationMs = 0;
    int64_t defaultFrameDurationMs = 33;

    // Codec extra data (e.g. SPS/PPS, codec private data).
    std::vector<uint8_t> extraData;

    // Video related fields.
    int width = 0;
    int height = 0;
    int pixelFormat = -1;
    int sampleAspectRatioNum = 1;
    int sampleAspectRatioDen = 1;
};

#endif // DECODERINITDATA_H
