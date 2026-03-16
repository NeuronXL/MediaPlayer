#ifndef IAUDIOADAPTER_H
#define IAUDIOADAPTER_H

#include "../common/frame.h"

#include <cstdint>
#include <memory>

struct SwrContext;

class IAudioAdapter {
public:
    IAudioAdapter();
    virtual ~IAudioAdapter();

    virtual bool start() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;

    virtual bool write(const std::shared_ptr<AudioFrame>& frame) = 0;

    virtual int64_t playedTimeUs() const = 0;

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
