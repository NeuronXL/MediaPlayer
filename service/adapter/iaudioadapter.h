#ifndef IAUDIOADAPTER_H
#define IAUDIOADAPTER_H

#include "../common/frame.h"
#include "../pipeline/interface/iaudiooutputadapter.h"

#include <cstdint>
#include <memory>

struct SwrContext;
class IAudioFrameSource;

struct AudioOutputSpec {
    int sampleRate = 0;
    int channels = 0;
    int sampleFormat = -1;
};

class IAudioAdapter : public IAudioOutputAdapter {
public:
    IAudioAdapter();
    virtual ~IAudioAdapter();

    bool start() override = 0;
    void pause() override = 0;
    void stop() override = 0;
    void reset() override = 0;
    void setAudioFrameSource(IAudioFrameSource* source) override = 0;
    AudioOutputSpec outputSpec() const override = 0;
    int64_t playedTimeMs() const override = 0;

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
