#include "filemediasource.h"

#include <filesystem>

FileMediaSource::FileMediaSource()
    : m_stream()
    , m_uri()
    , m_sizeBytes(0) {}

FileMediaSource::~FileMediaSource() {
    close();
}

bool FileMediaSource::open(const std::string& uri, std::string* errorMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    closeUnlocked();

    if (uri.empty()) {
        setError(errorMessage, "media source uri is empty");
        return false;
    }

    std::error_code ec;
    const std::filesystem::path path(uri);
    if (!std::filesystem::exists(path, ec) || ec) {
        setError(errorMessage, "media source file does not exist");
        return false;
    }
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        setError(errorMessage, "media source path is not a regular file");
        return false;
    }

    const auto fileSize = std::filesystem::file_size(path, ec);
    if (ec) {
        setError(errorMessage, "media source file_size failed");
        return false;
    }

    m_stream.open(uri, std::ios::in | std::ios::binary);
    if (!m_stream.is_open()) {
        setError(errorMessage, "media source open failed");
        return false;
    }

    m_uri = uri;
    m_sizeBytes = static_cast<int64_t>(fileSize);
    setError(errorMessage, "");
    return true;
}

void FileMediaSource::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    closeUnlocked();
}

void FileMediaSource::closeUnlocked() {
    if (m_stream.is_open()) {
        m_stream.close();
    }
    m_stream.clear();
    m_uri.clear();
    m_sizeBytes = 0;
}

bool FileMediaSource::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stream.is_open();
}

bool FileMediaSource::isSeekable() const {
    return true;
}

int64_t FileMediaSource::sizeBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sizeBytes;
}

int64_t FileMediaSource::tell() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_stream.is_open()) {
        return -1;
    }

    const auto pos = m_stream.tellg();
    if (pos == std::ifstream::pos_type(-1)) {
        return -1;
    }
    return static_cast<int64_t>(pos);
}

int FileMediaSource::read(std::uint8_t* buffer, int bufferSize, std::string* errorMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_stream.is_open()) {
        setError(errorMessage, "media source is not opened");
        return -1;
    }
    if (buffer == nullptr || bufferSize < 0) {
        setError(errorMessage, "invalid read buffer");
        return -1;
    }
    if (bufferSize == 0) {
        return 0;
    }

    m_stream.read(reinterpret_cast<char*>(buffer), bufferSize);
    const std::streamsize readSize = m_stream.gcount();
    if (readSize < 0) {
        setError(errorMessage, "media source read failed");
        return -1;
    }

    if (m_stream.bad()) {
        setError(errorMessage, "media source read failed");
        return -1;
    }

    setError(errorMessage, "");
    return static_cast<int>(readSize);
}

bool FileMediaSource::seek(int64_t offset, MediaSourceSeekOrigin origin, std::string* errorMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_stream.is_open()) {
        setError(errorMessage, "media source is not opened");
        return false;
    }

    std::ios_base::seekdir seekDir = std::ios::beg;
    switch (origin) {
    case MediaSourceSeekOrigin::Begin:
        seekDir = std::ios::beg;
        break;
    case MediaSourceSeekOrigin::Current:
        seekDir = std::ios::cur;
        break;
    case MediaSourceSeekOrigin::End:
        seekDir = std::ios::end;
        break;
    default:
        setError(errorMessage, "invalid seek origin");
        return false;
    }

    m_stream.clear();
    m_stream.seekg(offset, seekDir);
    if (m_stream.fail()) {
        setError(errorMessage, "media source seek failed");
        return false;
    }

    setError(errorMessage, "");
    return true;
}

void FileMediaSource::setError(std::string* errorMessage, const std::string& value) {
    if (errorMessage != nullptr) {
        *errorMessage = value;
    }
}
