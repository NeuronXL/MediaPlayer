#ifndef QAUDIOOUTPUTADAPTER_H
#define QAUDIOOUTPUTADAPTER_H

#include "iaudioadapter.h"
#include "../player/iaudioframesource.h"

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
    void setAudioFrameSource(IAudioFrameSource* source) override;
    AudioOutputSpec outputSpec() const override;
    int64_t playedTimeMs() const override;

private:
    bool ensureSink(const AudioFrame& frame);
    bool ensureResampler(const AudioFrame& frame);

private:
    QAudioFormat m_audioFormat;
    QAudioSink* m_audioSink;
    QIODevice* m_ioDevice;
    IAudioFrameSource* m_audioFrameSource;
};

#endif // QAUDIOOUTPUTADAPTER_H
