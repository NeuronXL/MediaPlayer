#ifndef MEDIASOURCEINFO_H
#define MEDIASOURCEINFO_H

#include <cstdint>
#include <string>

struct MediaSourceInfo {
    std::string filePath;
    std::string containerFormat;
    std::string videoCodec;
    int64_t durationMs = 0;
    int width = 0;
    int height = 0;
    double frameRate = 0.0;
};

#endif // MEDIASOURCEINFO_H
