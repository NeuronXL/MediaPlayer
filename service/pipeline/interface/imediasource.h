#ifndef IMEDIASOURCE_H
#define IMEDIASOURCE_H

#include <cstdint>
#include <string>

enum class MediaSourceSeekOrigin {
    Begin,
    Current,
    End
};

class IMediaSource {
public:
    virtual ~IMediaSource() = default;

    virtual bool open(const std::string& uri, std::string* errorMessage) = 0;
    virtual void close() = 0;

    virtual bool isOpen() const = 0;
    virtual bool isSeekable() const = 0;
    virtual int64_t sizeBytes() const = 0;
    virtual int64_t tell() const = 0;

    virtual int read(std::uint8_t* buffer, int bufferSize, std::string* errorMessage) = 0;
    virtual bool seek(int64_t offset, MediaSourceSeekOrigin origin, std::string* errorMessage) = 0;
};

#endif // IMEDIASOURCE_H
