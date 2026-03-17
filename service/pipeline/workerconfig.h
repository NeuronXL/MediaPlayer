#ifndef WORKERCONFIG_H
#define WORKERCONFIG_H

#include <cstdint>

struct AVCodecParameters;

struct VideoDecoderConfig
{
    AVCodecParameters* codecParameters = nullptr;
    int timeBaseNum = 0;
    int timeBaseDen = 1;
    int64_t frameIntervalMs = 33;
};

struct AudioDecoderConfig
{
    AVCodecParameters* codecParameters = nullptr;
    int timeBaseNum = 0;
    int timeBaseDen = 1;
    int64_t frameIntervalMs = 20;
};

#endif // WORKERCONFIG_H
