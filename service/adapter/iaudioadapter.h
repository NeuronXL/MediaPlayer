#ifndef IAUDIOADAPTER_H
#define IAUDIOADAPTER_H

#include "../common/frame.h"

#include <cstdint>
#include <memory>

struct SwrContext;
class IAudioFrameSource;

struct AudioOutputSpec {
    int sampleRate = 0;
    int channels = 0;
    int sampleFormat = -1;
};

class IAudioAdapter {
public:
    IAudioAdapter();
    virtual ~IAudioAdapter();

    virtual bool start() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void setAudioFrameSource(IAudioFrameSource* source) = 0;
    virtual AudioOutputSpec outputSpec() const = 0;

protected:
    void releaseAudioContext();

protected:
    SwrContext* m_swrContext;
    int m_inSampleRate;
    int m_inChannels;
    int m_inSampleFormat;
    int m_outSampleRate;
    int m_outChannels;
    int m_outSampleFormat;
};

#endif // IAUDIOADAPTER_H
