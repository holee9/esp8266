// @MX:NOTE: [MOCK] ESP8266 FS.h mock for native testing

#ifndef ARTHUR_FS_MOCK_H
#define ARTHUR_FS_MOCK_H

#include <cstdint>
#include <cstring>

#ifdef ARTHUR_NATIVE_TEST

// 파일 모드 상수
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

// 가상 파일 클래스
class File {
public:
    File() : _isOpen(false), _position(0), _size(0) {}
    ~File() {}

    operator bool() const {
        return _isOpen;
    }

    size_t read(uint8_t* buf, size_t len) {
        if (!_isOpen) return 0;
        size_t toRead = (len > (_size - _position)) ? (_size - _position) : len;
        _position += toRead;
        return toRead;
    }

    size_t write(const uint8_t* buf, size_t len) {
        if (!_isOpen) return 0;
        _position += len;
        if (_position > _size) _size = _position;
        return len;
    }

    int read() {
        if (!_isOpen || _position >= _size) return -1;
        return 0;  // Mock: return 0
    }

    size_t write(uint8_t c) {
        return write(&c, 1);
    }

    int peek() {
        if (!_isOpen || _position >= _size) return -1;
        return 0;  // Mock: return 0
    }

    void flush() {
        // Stub
    }

    bool seek(uint32_t pos) {
        if (!_isOpen) return false;
        if (pos > _size) return false;
        _position = pos;
        return true;
    }

    uint32_t position() const {
        return _position;
    }

    uint32_t size() const {
        return _size;
    }

    void close() {
        _isOpen = false;
    }

    const char* name() const {
        return "mock_file";
    }

    bool isDirectory() const {
        return false;
    }

    File openNextFile(const char* mode = FILE_READ) {
        return File();
    }

    void rewindDirectory() {
        // Stub
    }

    // Mock control
    void setOpen(bool open) { _isOpen = open; }
    void setSize(uint32_t size) { _size = size; }

private:
    bool _isOpen;
    uint32_t _position;
    uint32_t _size;
};

// 가상 파일 시스템 클래스
class FS {
public:
    FS() {}
    ~FS() {}

    bool begin() {
        return true;
    }

    void end() {
        // Stub
    }

    bool format() {
        return true;
    }

    File open(const char* path, const char* mode = FILE_READ) {
        File f;
        f.setOpen(true);
        return f;
    }

    File open(const String& path, const char* mode = FILE_READ) {
        return open(path.c_str(), mode);
    }

    bool exists(const char* path) {
        return false;  // Mock: file doesn't exist
    }

    bool exists(const String& path) {
        return exists(path.c_str());
    }

    bool remove(const char* path) {
        return true;
    }

    bool remove(const String& path) {
        return remove(path.c_str());
    }

    bool rename(const char* pathFrom, const char* pathTo) {
        return true;
    }

    bool rename(const String& pathFrom, const String& pathTo) {
        return rename(pathFrom.c_str(), pathTo.c_str());
    }

    bool mkdir(const char* path) {
        return true;
    }

    bool rmdir(const char* path) {
        return true;
    }

    uint32_t totalBytes() {
        return 1024 * 1024;  // 1MB mock
    }

    uint32_t usedBytes() {
        return 0;
    }

    uint32_t blockSize() {
        return 4096;
    }
};

// 전역 파일 시스템 인스턴스
extern FS LittleFS;
extern FS SPIFFS;

#endif // ARTHUR_NATIVE_TEST
#endif // ARTHUR_FS_MOCK_H
