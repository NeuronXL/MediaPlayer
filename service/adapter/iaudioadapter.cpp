#include "iaudioadapter.h"

extern "C" {
#include <libswresample/swresample.h>
}

IAudioAdapter::IAudioAdapter()
    : m_swrContext(nullptr)
    , m_inSampleRate(0)
    , m_inChannels(0)
    , m_inSampleFormat(-1)
    , m_outSampleRate(0)
    , m_outChannels(0)
    , m_outSampleFormat(-1)
{
}

IAudioAdapter::~IAudioAdapter()
{
    releaseAudioContext();
}

void IAudioAdapter::releaseAudioContext()
{
    if (m_swrContext != nullptr) {
        swr_free(&m_swrContext);
    }
    m_inSampleRate = 0;
    m_inChannels = 0;
    m_inSampleFormat = -1;
    m_outSampleRate = 0;
    m_outChannels = 0;
    m_outSampleFormat = -1;
}
