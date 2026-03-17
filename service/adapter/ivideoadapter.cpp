#include "ivideoadapter.h"

extern "C" {
#include <libswscale/swscale.h>
}

IVideoAdapter::IVideoAdapter()
    : m_swsContext(nullptr)
    , m_srcWidth(0)
    , m_srcHeight(0)
    , m_srcFormat(-1)
    , m_dstWidth(0)
    , m_dstHeight(0)
    , m_dstFormat(-1)
{
}

IVideoAdapter::~IVideoAdapter()
{
    releaseVideoContext();
}

void IVideoAdapter::releaseVideoContext()
{
    if (m_swsContext != nullptr) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    m_srcWidth = 0;
    m_srcHeight = 0;
    m_srcFormat = -1;
    m_dstWidth = 0;
    m_dstHeight = 0;
    m_dstFormat = -1;
}
