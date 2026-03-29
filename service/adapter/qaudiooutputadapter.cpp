#include "qaudiooutputadapter.h"

#include <QMediaDevices>
#include <QSpan>

#include <cstdint>
#include <cstring>

extern "C" {
#include <libavutil/samplefmt.h>
}

QAudioOutputAdapter::QAudioOutputAdapter()
    : m_audioSink(nullptr), m_ioDevice(nullptr), m_audioFrameSource(nullptr) {}

QAudioOutputAdapter::~QAudioOutputAdapter() {
    stop();
}

bool QAudioOutputAdapter::start() {
    if (m_audioFrameSource == nullptr) {
        return false;
    }

    if (m_audioSink == nullptr) {
        const AudioOutputSpec spec = outputSpec();
        m_audioFormat = QAudioFormat{};
        m_audioFormat.setSampleRate(spec.sampleRate);
        m_audioFormat.setChannelCount(spec.channels);
        m_audioFormat.setSampleFormat(QAudioFormat::Int16);

        const auto outputDevice = QMediaDevices::defaultAudioOutput();
        if (!outputDevice.isFormatSupported(m_audioFormat)) {
            return false;
        }
        m_audioSink = new QAudioSink(outputDevice, m_audioFormat);
        if (m_audioSink == nullptr) {
            return false;
        }
    }

    if (m_audioSink->state() == QtAudio::ActiveState || m_audioSink->state() == QtAudio::IdleState) {
        return true;
    }

    if (m_audioSink->state() == QtAudio::SuspendedState) {
        m_audioSink->resume();
        return true;
    }

    m_audioSink->start([this](QSpan<qint16> interleavedAudioBuffer) {
        const int requestedBytes = static_cast<int>(interleavedAudioBuffer.size_bytes());
        if (requestedBytes <= 0) {
            return;
        }

        auto* destination = reinterpret_cast<std::uint8_t*>(interleavedAudioBuffer.data());
        int filledBytes = 0;
        if (m_audioFrameSource != nullptr) {
            filledBytes = m_audioFrameSource->readPcm(destination, requestedBytes);
        }
        if (filledBytes < 0) {
            filledBytes = 0;
        }
        if (filledBytes > requestedBytes) {
            filledBytes = requestedBytes;
        }
        if (filledBytes < requestedBytes) {
            std::memset(destination + filledBytes, 0, static_cast<size_t>(requestedBytes - filledBytes));
        }
    });

    m_ioDevice = nullptr;
    return m_audioSink->error() == QtAudio::NoError;
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

void QAudioOutputAdapter::setAudioFrameSource(IAudioFrameSource* source) {
    m_audioFrameSource = source;
}

AudioOutputSpec QAudioOutputAdapter::outputSpec() const {
    AudioOutputSpec spec;
    const auto outputDevice = QMediaDevices::defaultAudioOutput();
    const QAudioFormat preferredFormat = outputDevice.preferredFormat();
    spec.sampleRate = preferredFormat.sampleRate() > 0
        ? preferredFormat.sampleRate()
        : (m_outSampleRate > 0 ? m_outSampleRate : 48000);
    spec.channels = preferredFormat.channelCount() > 0
        ? preferredFormat.channelCount()
        : (m_outChannels > 0 ? m_outChannels : 2);
    spec.sampleFormat = AV_SAMPLE_FMT_S16;
    return spec;
}

bool QAudioOutputAdapter::ensureSink(const AudioFrame& frame) {
    if (frame.sampleRate > 0) {
        m_outSampleRate = frame.sampleRate;
    }
    if (frame.channels > 0) {
        m_outChannels = frame.channels;
    }
    return true;
}

bool QAudioOutputAdapter::ensureResampler(const AudioFrame& frame) {
    if (frame.sampleRate <= 0 || frame.channels <= 0) {
        return false;
    }
    m_inSampleRate = frame.sampleRate;
    m_inChannels = frame.channels;
    m_inSampleFormat = frame.format;
    m_outSampleRate = frame.sampleRate;
    m_outChannels = frame.channels;
    m_outSampleFormat = frame.format;
    return true;
}
