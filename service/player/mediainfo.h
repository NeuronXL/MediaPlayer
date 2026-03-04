#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <QMetaType>
#include <QString>

struct MediaInfo
{
    QString filePath;
    QString containerFormat;
    QString videoCodec;
    qint64 durationMs = 0;
    int width = 0;
    int height = 0;
    double frameRate = 0.0;
};

Q_DECLARE_METATYPE(MediaInfo)

#endif // MEDIAINFO_H
