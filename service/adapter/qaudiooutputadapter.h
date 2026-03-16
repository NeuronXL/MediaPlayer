#ifndef QAUDIOOUTPUTADAPTER_H
#define QAUDIOOUTPUTADAPTER_H

#include "iaudioadapter.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>

class QAudioOutputAdapter final : public IAudioAdapter {
public:
    QAudioOutputAdapter();
    ~QAudioOutputAdapter() override;

    bool start() override;
    void pause() override;
    void stop() override;
    void reset() override;
    bool write(const std::shared_ptr<AudioFrame>& frame) override;
    int64_t playedTimeUs() const override;

private:
    bool ensureSink(const AudioFrame& frame);
    bool ensureResampler(const AudioFrame& frame);

private:
    QAudioFormat m_audioFormat;
    QAudioSink* m_audioSink;
    QIODevice* m_ioDevice;
};

#endif // QAUDIOOUTPUTADAPTER_H
