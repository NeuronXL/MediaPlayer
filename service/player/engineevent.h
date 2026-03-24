#ifndef ENGINEEVENT_H
#define ENGINEEVENT_H

#include "../common/frame.h"
#include "../pipeline/mediasourceinfo.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

struct VideoFrameEvent {
    std::shared_ptr<VideoFrame> frame;
};

struct OpenMediaSucceededEvent {
    MediaSourceInfo mediaInfo;
};

struct OpenMediaFailedEvent {
    std::string filePath;
    std::string errorMessage;
};

using EngineEvent = std::variant<VideoFrameEvent, OpenMediaSucceededEvent, OpenMediaFailedEvent>;

enum class EngineEventType : std::uint32_t {
    VideoFrame = 1u << 0,
    OpenMediaSucceeded = 1u << 1,
    OpenMediaFailed = 1u << 2
};

using EngineEventMask = std::uint32_t;

constexpr EngineEventMask ENGINE_EVENT_VIDEO_FRAME = static_cast<EngineEventMask>(EngineEventType::VideoFrame);
constexpr EngineEventMask ENGINE_EVENT_OPEN_MEDIA_SUCCEEDED = static_cast<EngineEventMask>(EngineEventType::OpenMediaSucceeded);
constexpr EngineEventMask ENGINE_EVENT_OPEN_MEDIA_FAILED = static_cast<EngineEventMask>(EngineEventType::OpenMediaFailed);
constexpr EngineEventMask ENGINE_EVENT_ALL =
    ENGINE_EVENT_VIDEO_FRAME | ENGINE_EVENT_OPEN_MEDIA_SUCCEEDED | ENGINE_EVENT_OPEN_MEDIA_FAILED;

inline EngineEventMask eventMaskOf(const EngineEvent& event) {
    if (std::holds_alternative<VideoFrameEvent>(event)) {
        return ENGINE_EVENT_VIDEO_FRAME;
    }
    if (std::holds_alternative<OpenMediaSucceededEvent>(event)) {
        return ENGINE_EVENT_OPEN_MEDIA_SUCCEEDED;
    }
    return ENGINE_EVENT_OPEN_MEDIA_FAILED;
}

#endif // ENGINEEVENT_H
