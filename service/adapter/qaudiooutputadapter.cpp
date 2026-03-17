#include "qaudiooutputadapter.h"

#include <QMediaDevices>
#include <QThread>

#include <vector>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

QAudioOutputAdapter::QAudioOutputAdapter()
    : m_audioSink(nullptr), m_ioDevice(nullptr) {}

QAudioOutputAdapter::~QAudioOutputAdapter() {
    stop();
}

bool QAudioOutputAdapter::start() {
    if (m_audioSink == nullptr) {
        return false;
    }

    if (m_ioDevice == nullptr) {
        m_ioDevice = m_audioSink->start();
    } else {
        m_audioSink->resume();
    }
    return m_ioDevice != nullptr;
}

void QAudioOutputAdapter::pause() {
    if (m_audioSink != nullptr) {
        m_audioSink->suspend();
    }
}

void QAudioOutputAdapter::stop() {
    if (m_audioSink != nullptr) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    m_ioDevice = nullptr;
    m_audioFormat = QAudioFormat{};
    releaseAudioContext();
}

void QAudioOutputAdapter::reset() {
    stop();
}

bool QAudioOutputAdapter::write(const std::shared_ptr<AudioFrame>& frame) {
    if (!frame || frame->frame == nullptr || frame->nbSamples <= 0 || frame->channels <= 0) {
        return false;
    }

    if (!ensureResampler(*frame)) {
        return false;
    }

    if (!ensureSink(*frame)) {
        return false;
    }

    if (!start()) {
        return false;
    }

    const int outputBytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(m_outSampleFormat));
    if (outputBytesPerSample <= 0) {
        return false;
    }

    const int outputSamples = static_cast<int>(
        av_rescale_rnd(swr_get_delay(m_swrContext, frame->sampleRate) + frame->nbSamples,
                       m_outSampleRate,
                       frame->sampleRate,
                       AV_ROUND_UP));
    if (outputSamples <= 0 || m_outChannels <= 0) {
        return false;
    }

    std::vector<uint8_t> outputBuffer(static_cast<size_t>(outputSamples) * m_outChannels * outputBytesPerSample);
    uint8_t* outputData[1] = {outputBuffer.data()};
    const uint8_t* const* inputData = const_cast<const uint8_t* const*>(frame->frame->extended_data);
    const int convertedSamples = swr_convert(
        m_swrContext,
        outputData,
        outputSamples,
        inputData,
        frame->nbSamples);
    if (convertedSamples <= 0) {
        return false;
    }

    const qint64 totalBytes = static_cast<qint64>(convertedSamples) * m_outChannels * outputBytesPerSample;
    const char* data = reinterpret_cast<const char*>(outputBuffer.data());
    qint64 written = 0;
    while (written < totalBytes) {
        const qint64 once = m_ioDevice->write(data + written, totalBytes - written);
        if (once < 0) {
            return false;
        }
        if (once == 0) {
            QThread::msleep(2);
            continue;
        }
        written += once;
    }

    return true;
}

int64_t QAudioOutputAdapter::playedTimeMs() const {
    if (m_audioSink == nullptr) {
        return 0;
    }
    return m_audioSink->processedUSecs() / 1000;
}

bool QAudioOutputAdapter::ensureSink(const AudioFrame& frame) {
    Q_UNUSED(frame);
    const QAudioFormat::SampleFormat sampleFormat = QAudioFormat::Int16;
    if (sampleFormat == QAudioFormat::Unknown) {
        return false;
    }

    const bool sameFormat = m_audioSink != nullptr &&
                            m_audioFormat.sampleRate() == m_outSampleRate &&
                            m_audioFormat.channelCount() == m_outChannels &&
                            m_audioFormat.sampleFormat() == sampleFormat;
    if (sameFormat) {
        return true;
    }

    if (m_audioSink != nullptr) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    m_ioDevice = nullptr;
    m_audioFormat = QAudioFormat{};

    m_audioFormat.setSampleRate(m_outSampleRate);
    m_audioFormat.setChannelCount(m_outChannels);
    m_audioFormat.setSampleFormat(sampleFormat);

    auto outputDevice = QMediaDevices::defaultAudioOutput();
    if (!outputDevice.isFormatSupported(m_audioFormat)) {
        return false;
    }

    m_audioSink = new QAudioSink(outputDevice, m_audioFormat);
    return m_audioSink != nullptr;
}

bool QAudioOutputAdapter::ensureResampler(const AudioFrame& frame) {
    const int inSampleRate = frame.sampleRate;
    const int inChannels = frame.channels;
    const int inSampleFormat = frame.format;
    const int outSampleRate = frame.sampleRate;
    const int outChannels = frame.channels;
    const int outSampleFormat = AV_SAMPLE_FMT_S16;

    if (inSampleRate <= 0 || inChannels <= 0) {
        return false;
    }

    const bool sameConfig = m_swrContext != nullptr &&
                            m_inSampleRate == inSampleRate &&
                            m_inChannels == inChannels &&
                            m_inSampleFormat == inSampleFormat &&
                            m_outSampleRate == outSampleRate &&
                            m_outChannels == outChannels &&
                            m_outSampleFormat == outSampleFormat;
    if (sameConfig) {
        return true;
    }

    releaseAudioContext();

    AVChannelLayout inLayout;
    AVChannelLayout outLayout;
    av_channel_layout_default(&inLayout, inChannels);
    av_channel_layout_default(&outLayout, outChannels);

    if (swr_alloc_set_opts2(&m_swrContext,
                            &outLayout,
                            static_cast<AVSampleFormat>(outSampleFormat),
                            outSampleRate,
                            &inLayout,
                            static_cast<AVSampleFormat>(inSampleFormat),
                            inSampleRate,
                            0,
                            nullptr) < 0) {
        av_channel_layout_uninit(&inLayout);
        av_channel_layout_uninit(&outLayout);
        releaseAudioContext();
        return false;
    }

    av_channel_layout_uninit(&inLayout);
    av_channel_layout_uninit(&outLayout);

    if (swr_init(m_swrContext) < 0) {
        releaseAudioContext();
        return false;
    }

    m_inSampleRate = inSampleRate;
    m_inChannels = inChannels;
    m_inSampleFormat = inSampleFormat;
    m_outSampleRate = outSampleRate;
    m_outChannels = outChannels;
    m_outSampleFormat = outSampleFormat;
    return true;
}
