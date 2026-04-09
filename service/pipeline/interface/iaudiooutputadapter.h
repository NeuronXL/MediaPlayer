#ifndef IAUDIOOUTPUTADAPTER_H
#define IAUDIOOUTPUTADAPTER_H

#include <cstdint>

class IAudioFrameSource;

struct AudioOutputSpec;

class IAudioOutputAdapter {
public:
    virtual ~IAudioOutputAdapter() = default;

    virtual bool start() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void setAudioFrameSource(IAudioFrameSource* source) = 0;
    virtual AudioOutputSpec outputSpec() const = 0;
    virtual int64_t playedTimeMs() const = 0;
};

#endif // IAUDIOOUTPUTADAPTER_H
