#ifndef ISUBTITLERENDERADAPTER_H
#define ISUBTITLERENDERADAPTER_H

#include <cstdint>
#include <string>

struct SubtitleRenderData {
    int64_t startMs = 0;
    int64_t endMs = 0;
    std::string text;
};

class ISubtitleRenderAdapter {
public:
    virtual ~ISubtitleRenderAdapter() = default;
    virtual void onSubtitle(const SubtitleRenderData& subtitle) = 0;
};

#endif // ISUBTITLERENDERADAPTER_H
