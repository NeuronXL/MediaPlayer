#ifndef IVIDEORENDERADAPTER_H
#define IVIDEORENDERADAPTER_H

#include "../../common/frame.h"

#include <memory>

class IVideoRenderAdapter {
public:
    virtual ~IVideoRenderAdapter() = default;
    virtual void onVideoFrame(const std::shared_ptr<VideoFrame>& frame) = 0;
};

#endif // IVIDEORENDERADAPTER_H
