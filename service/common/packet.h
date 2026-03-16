#ifndef PACKET_H
#define PACKET_H
#include <stdint.h>

struct AVPacket;

enum class TrackType { Video, Audio, Subtitle };

class Packet {
    public:
    TrackType track = TrackType::Video;
    AVPacket* packet = nullptr;
    int serial;
    double pts;
    double duration;
    int64_t pos;
};

#endif // PACKET_H
