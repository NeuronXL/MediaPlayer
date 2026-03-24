#include "ffmpegdemuxer.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "../logging/logservice.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/codec_desc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

FFmpegDemuxer::FFmpegDemuxer(PacketQueue* videoPacketQueue, PacketQueue* audioPacketQueue,
                             PacketQueue* subtitlePacketQueue, FrameQueue* videoFrameQueue,
                             FrameQueue* audioFrameQueue)
    : m_videoPacketQueue(videoPacketQueue), m_audioPacketQueue(audioPacketQueue),
      m_subtitlePacketQueue(subtitlePacketQueue), m_videoFrameQueue(videoFrameQueue),
      m_audioFrameQueue(audioFrameQueue), m_formatContext(nullptr),
      m_videoStreamIndex(-1), m_audioStreamIndex(-1), m_subtitleStreamIndex(-1),
      m_videoDecoder(videoPacketQueue, videoFrameQueue),
      m_audioDecoder(audioPacketQueue, audioFrameQueue) {}

FFmpegDemuxer::~FFmpegDemuxer() {}

bool FFmpegDemuxer::open(const std::string& filePath, MediaSourceInfo* sourceInfo, std::string* errorMessage) {
    namespace fs = std::filesystem;
    const fs::path fsPath(filePath);
    if (sourceInfo != nullptr) {
        *sourceInfo = MediaSourceInfo{};
    }
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    try {
        if (filePath.empty()) {
            throw std::runtime_error("File path is empty.");
        }

        std::error_code ec;
        if (!fs::exists(fsPath, ec) || ec) {
            throw std::runtime_error("File does not exist.");
        }
        if (!fs::is_regular_file(fsPath, ec) || ec) {
            throw std::runtime_error("Selected path is not a file.");
        }

        std::ifstream readable(filePath, std::ios::binary);
        if (!readable.good()) {
            throw std::runtime_error("File is not readable.");
        }
        readable.close();

        int ret = avformat_open_input(&m_formatContext, filePath.c_str(), nullptr, nullptr);
        if (ret < 0) {
            throw std::runtime_error("avformat_open_input failed.");
        }

        ret = avformat_find_stream_info(m_formatContext, nullptr);
        if (ret < 0) {
            throw std::runtime_error("avformat_find_stream_info failed.");
        }

        av_dump_format(m_formatContext, 0, filePath.c_str(), 0);

        std::vector<int> videoStreams;
        std::vector<int> audioStreams;
        std::vector<int> subtitleStreams;
        for (unsigned i = 0; i < m_formatContext->nb_streams; ++i) {
            AVStream* s = m_formatContext->streams[i];
            if (!s || !s->codecpar) {
                continue;
            }

            switch (s->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                videoStreams.push_back(static_cast<int>(i));
                break;
            case AVMEDIA_TYPE_AUDIO:
                audioStreams.push_back(static_cast<int>(i));
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                subtitleStreams.push_back(static_cast<int>(i));
                break;
            default:
                break;
            }
        }

        m_videoStreamIndex = videoStreams.empty() ? -1 : videoStreams.front();
        m_audioStreamIndex = audioStreams.empty() ? -1 : audioStreams.front();
        m_subtitleStreamIndex = subtitleStreams.empty() ? -1 : subtitleStreams.front();

        if (m_videoStreamIndex < 0 ||
            m_videoStreamIndex >= static_cast<int>(m_formatContext->nb_streams)) {
            throw std::runtime_error("No valid video stream found.");
        }

        AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
        if (videoStream == nullptr || videoStream->codecpar == nullptr) {
            throw std::runtime_error("Invalid video stream.");
        }

        AVCodecParameters* codecParameters = avcodec_parameters_alloc();
        if (codecParameters == nullptr) {
            throw std::runtime_error("Failed to allocate codec parameters.");
        }

        ret = avcodec_parameters_copy(codecParameters, videoStream->codecpar);
        if (ret < 0) {
            avcodec_parameters_free(&codecParameters);
            throw std::runtime_error("Failed to copy codec parameters.");
        }

        VideoDecoderConfig config;
        config.codecParameters = codecParameters;
        config.timeBaseNum = videoStream->time_base.num;
        config.timeBaseDen = videoStream->time_base.den > 0 ? videoStream->time_base.den : 1;
        if (videoStream->avg_frame_rate.num > 0 && videoStream->avg_frame_rate.den > 0) {
            const double fps = av_q2d(videoStream->avg_frame_rate);
            config.frameIntervalMs = fps > 0.0 ? static_cast<int64_t>(1000.0 / fps) : 33;
        } else {
            config.frameIntervalMs = 33;
        }
        if (sourceInfo != nullptr) {
            sourceInfo->filePath = filePath;
            sourceInfo->containerFormat =
                (m_formatContext != nullptr && m_formatContext->iformat != nullptr && m_formatContext->iformat->name != nullptr)
                    ? m_formatContext->iformat->name
                    : "";
            sourceInfo->videoCodec = videoStream->codecpar != nullptr ? avcodec_get_name(videoStream->codecpar->codec_id) : "";
            sourceInfo->durationMs = (m_formatContext != nullptr && m_formatContext->duration > 0)
                ? (m_formatContext->duration / (AV_TIME_BASE / 1000))
                : 0;
            sourceInfo->width = videoStream->codecpar != nullptr ? videoStream->codecpar->width : 0;
            sourceInfo->height = videoStream->codecpar != nullptr ? videoStream->codecpar->height : 0;
            sourceInfo->frameRate = (videoStream->avg_frame_rate.num > 0 && videoStream->avg_frame_rate.den > 0)
                ? av_q2d(videoStream->avg_frame_rate)
                : 0.0;
        }

        try {
            m_videoDecoder.configure(config);
            avcodec_parameters_free(&config.codecParameters);
        } catch (const std::exception&) {
            avcodec_parameters_free(&config.codecParameters);
            resetDemuxSession();
            if (errorMessage != nullptr) {
                *errorMessage = "video decoder configure failed";
            }
            return false;
        }

        if (m_audioStreamIndex >= 0 &&
            m_audioStreamIndex < static_cast<int>(m_formatContext->nb_streams)) {
            AVStream* audioStream = m_formatContext->streams[m_audioStreamIndex];
            if (audioStream != nullptr && audioStream->codecpar != nullptr) {
                AVCodecParameters* audioCodecParameters = avcodec_parameters_alloc();
                if (audioCodecParameters == nullptr) {
                    resetDemuxSession();
                    if (errorMessage != nullptr) {
                        *errorMessage = "failed to allocate audio codec parameters";
                    }
                    return false;
                }

                ret = avcodec_parameters_copy(audioCodecParameters, audioStream->codecpar);
                if (ret < 0) {
                    avcodec_parameters_free(&audioCodecParameters);
                    resetDemuxSession();
                    if (errorMessage != nullptr) {
                        *errorMessage = "failed to copy audio codec parameters";
                    }
                    return false;
                }

                AudioDecoderConfig audioConfig;
                audioConfig.codecParameters = audioCodecParameters;
                audioConfig.timeBaseNum = audioStream->time_base.num;
                audioConfig.timeBaseDen = audioStream->time_base.den > 0 ? audioStream->time_base.den : 1;
                audioConfig.frameIntervalMs = 20;

                try {
                    m_audioDecoder.configure(audioConfig);
                    avcodec_parameters_free(&audioConfig.codecParameters);
                } catch (const std::exception&) {
                    avcodec_parameters_free(&audioConfig.codecParameters);
                    resetDemuxSession();
                    if (errorMessage != nullptr) {
                        *errorMessage = "audio decoder configure failed";
                    }
                    return false;
                }
            }
        }
        m_workThread = std::thread(&FFmpegDemuxer::runLoop, this);
        return true;
    } catch (const std::exception& ex) {
        resetDemuxSession();
        if (errorMessage != nullptr) {
            *errorMessage = ex.what();
        }
        return false;
    } catch (...) {
        resetDemuxSession();
        if (errorMessage != nullptr) {
            *errorMessage = "unknown error";
        }
        return false;
    }
}

void FFmpegDemuxer::seek(int64_t positionMs) {
    (void)positionMs;
}

void FFmpegDemuxer::runLoop() {
    if (m_formatContext == nullptr) {
        return;
    }

    while (m_formatContext != nullptr) {
        AVPacket* packet = av_packet_alloc();
        if (packet == nullptr) {
            return;
        }

        const int readResult = av_read_frame(m_formatContext, packet);
        if (readResult < 0) {
            av_packet_free(&packet);
            return;
        }

        PacketQueue* targetQueue = nullptr;
        TrackType trackType = TrackType::Video;
        if (packet->stream_index == m_videoStreamIndex) {
            targetQueue = m_videoPacketQueue;
            trackType = TrackType::Video;
        } else if (packet->stream_index == m_audioStreamIndex) {
            targetQueue = m_audioPacketQueue;
            trackType = TrackType::Audio;
        } else if (packet->stream_index == m_subtitleStreamIndex) {
            targetQueue = m_subtitlePacketQueue;
            trackType = TrackType::Subtitle;
        } else {
            av_packet_free(&packet);
            continue;
        }

        if (targetQueue == nullptr) {
            av_packet_free(&packet);
            continue;
        }

        Packet* mediaPacket = new Packet();
        mediaPacket->track = trackType;
        mediaPacket->packet = packet;
        const int64_t timestamp = packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts;
        if (timestamp != AV_NOPTS_VALUE && packet->stream_index >= 0 &&
            packet->stream_index < static_cast<int>(m_formatContext->nb_streams)) {
            const AVStream* stream = m_formatContext->streams[packet->stream_index];
            if (stream != nullptr) {
                mediaPacket->pts = av_rescale_q(timestamp, stream->time_base, AVRational{1, 1000});
            }
        }

        if (!targetQueue->push(mediaPacket)) {
            av_packet_free(&packet);
            return;
        }
    }
}

void FFmpegDemuxer::shutDown() {
    if (m_workThread.joinable()) {
        if (m_workThread.get_id() != std::this_thread::get_id()) {
            m_workThread.join();
        } else {
            m_workThread.detach();
        }
    }
    resetDemuxSession();
}

void FFmpegDemuxer::resetDemuxSession() {
    if (m_formatContext != nullptr) {
        avformat_close_input(&m_formatContext);
    }
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_subtitleStreamIndex = -1;
    m_videoStreams = {};
    m_audioStreams = {};
    m_subtitleStreams = {};
}
