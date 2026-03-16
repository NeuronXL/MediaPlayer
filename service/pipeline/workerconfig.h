#ifndef WORKERCONFIG_H
#define WORKERCONFIG_H

#include <cstdint>

struct AVCodecParameters;

struct VideoDecoderConfig
{
    AVCodecParameters* codecParameters = nullptr;
    int timeBaseNum = 0;
    int timeBaseDen = 1;
    int64_t frameIntervalUs = 33000;
};

struct AudioDecoderConfig
{
    AVCodecParameters* codecParameters = nullptr;
    int timeBaseNum = 0;
    int timeBaseDen = 1;
    int64_t frameIntervalUs = 20000;
};

#endif // WORKERCONFIG_H
