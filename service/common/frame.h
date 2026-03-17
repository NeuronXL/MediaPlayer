#ifndef FRAME_H
#define FRAME_H
#include <stdint.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/rational.h>
}

class Frame {
    public:
    Frame() {
        serial = -1;
        pts = -1;
        duration = -1;
        pos = -1;
    }
    virtual ~Frame() {}

    int serial;
    int64_t pts;
    int64_t duration;
    int64_t pos;
};

class VideoFrame : public Frame {
    public:
    VideoFrame() {
        frame = nullptr;
        width = 0;
        height = 0;
        format = -1;
    }
    ~VideoFrame() {
        if (frame) {
            av_frame_free(&frame);
            frame = nullptr;
        }
    }

    public:
    AVFrame* frame;
    int width;
    int height;
    int format;
    AVRational sar;
};

class AudioFrame : public Frame {
    public:
    AudioFrame() {
        frame = nullptr;
        sampleRate = 0;
        channels = 0;
        format = -1;
        nbSamples = 0;
    }
    ~AudioFrame() {
        if (frame) {
            av_frame_free(&frame);
            frame = nullptr;
        }
    }

    public:
    AVFrame* frame;
    int sampleRate;
    int channels;
    int format;
    int nbSamples;
};

#endif // FRAME_H
