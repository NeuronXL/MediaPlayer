#ifndef IAUDIOFRAMESOURCE_H
#define IAUDIOFRAMESOURCE_H

#include <cstdint>

class IAudioFrameSource {
public:
    virtual ~IAudioFrameSource() = default;

    virtual int readPcm(std::uint8_t* destination, int maxBytes) = 0;
};

#endif // IAUDIOFRAMESOURCE_H
