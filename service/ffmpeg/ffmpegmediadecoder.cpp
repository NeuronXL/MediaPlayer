#include "ffmpegmediadecoder.h"

#include <QFileInfo>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

FFmpegMediaDecoder::FFmpegMediaDecoder(QObject* parent)
    : QObject(parent)
{
}

FFmpegMediaDecoder::~FFmpegMediaDecoder() = default;

QString FFmpegMediaDecoder::currentMediaPath() const
{
    return m_currentMediaPath;
}

bool FFmpegMediaDecoder::hasOpenedMedia() const
{
    return !m_currentMediaPath.isEmpty();
}

void FFmpegMediaDecoder::openMedia(const QString& filePath)
{
    if (filePath.isEmpty()) {
        emit mediaOpenFailed(filePath, tr("File path is empty."));
        return;
    }

    emit mediaOpenStarted(filePath);

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit mediaOpenFailed(filePath, tr("File does not exist."));
        return;
    }

    if (!fileInfo.isFile()) {
        emit mediaOpenFailed(filePath, tr("Selected path is not a file."));
        return;
    }

    if (!fileInfo.isReadable()) {
        emit mediaOpenFailed(filePath, tr("File is not readable."));
        return;
    }

    if (!isSupportedVideoFile(filePath)) {
        emit mediaOpenFailed(filePath, tr("File type is not supported."));
        return;
    }

    if (m_currentMediaPath == filePath) {
        emit mediaOpened(filePath);
        return;
    }

    AVFormatContext* fmt = nullptr;
    int ret =
        avformat_open_input(&fmt, filePath.toStdString().c_str(), nullptr, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("avformat_open_input failed."));
        return;
    }

    ret = avformat_find_stream_info(fmt, nullptr);
    if (ret < 0) {
        emit mediaOpenFailed(filePath, tr("avformat_find_stream_info failed."));
        avformat_close_input(&fmt);
        return;
    }

    av_dump_format(fmt, 0, filePath.toStdString().c_str(), 0);
    avformat_close_input(&fmt);

    m_currentMediaPath = filePath;
    emit currentMediaPathChanged(m_currentMediaPath);
    emit mediaOpened(m_currentMediaPath);
}

void FFmpegMediaDecoder::closeMedia()
{
    if (m_currentMediaPath.isEmpty()) {
        return;
    }

    m_currentMediaPath.clear();
    emit currentMediaPathChanged(m_currentMediaPath);
}

bool FFmpegMediaDecoder::isSupportedVideoFile(const QString& filePath) const
{
    static const QStringList supportedExtensions = {
        QStringLiteral("mp4"),  QStringLiteral("mkv"),  QStringLiteral("avi"),
        QStringLiteral("mov"),  QStringLiteral("wmv"),  QStringLiteral("flv"),
        QStringLiteral("webm"), QStringLiteral("m4v"),  QStringLiteral("ts"),
        QStringLiteral("mts"),  QStringLiteral("m2ts"), QStringLiteral("mpeg"),
        QStringLiteral("mpg"),  QStringLiteral("3gp"),  QStringLiteral("ogv")};

    const QString suffix = QFileInfo(filePath).suffix();
    return supportedExtensions.contains(suffix, Qt::CaseInsensitive);
}
