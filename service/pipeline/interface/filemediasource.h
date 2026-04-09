#ifndef FILEMEDIASOURCE_H
#define FILEMEDIASOURCE_H

#include "imediasource.h"

#include <fstream>
#include <mutex>

class FileMediaSource : public IMediaSource {
public:
    FileMediaSource();
    ~FileMediaSource() override;

    bool open(const std::string& uri, std::string* errorMessage) override;
    void close() override;

    bool isOpen() const override;
    bool isSeekable() const override;
    int64_t sizeBytes() const override;
    int64_t tell() const override;

    int read(std::uint8_t* buffer, int bufferSize, std::string* errorMessage) override;
    bool seek(int64_t offset, MediaSourceSeekOrigin origin, std::string* errorMessage) override;

private:
    void closeUnlocked();
    static void setError(std::string* errorMessage, const std::string& value);

private:
    mutable std::mutex m_mutex;
    mutable std::ifstream m_stream;
    std::string m_uri;
    int64_t m_sizeBytes;
};

#endif // FILEMEDIASOURCE_H
