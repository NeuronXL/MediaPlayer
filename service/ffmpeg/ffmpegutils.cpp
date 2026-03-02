#include "ffmpegutils.h"

namespace FFmpegUtils
{

QString errorToString(int errorCode)
{
    Q_UNUSED(errorCode);
    // TODO: Translate FFmpeg error codes into readable messages.
    return {};
}

} // namespace FFmpegUtils
