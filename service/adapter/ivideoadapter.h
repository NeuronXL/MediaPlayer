#ifndef IVIDEOADAPTER_H
#define IVIDEOADAPTER_H

#include "../common/frame.h"

#include <memory>

struct SwsContext;

class IVideoAdapter {
public:
    IVideoAdapter();
    virtual ~IVideoAdapter();
    virtual void onVideoFrame(const std::shared_ptr<VideoFrame>& frame) = 0;

protected:
    void releaseVideoContext();

protected:
    SwsContext* m_swsContext;
    int m_srcWidth;
    int m_srcHeight;
    int m_srcFormat;
    int m_dstWidth;
    int m_dstHeight;
    int m_dstFormat;
};

#endif // IVIDEOADAPTER_H
