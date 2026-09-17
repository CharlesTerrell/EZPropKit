#pragma once

#include <string>
#include <unordered_map>
#include <cstring>
#include <algorithm>
#include <cstdint>

class File {
public:
    File() : _pos(0), _valid(false) {}
    File(const std::string& content) : _content(content), _pos(0), _valid(true) {}

    operator bool() const { return _valid; }

    int available() {
        if (!_valid || _pos >= _content.size()) return 0;
        return (int)(_content.size() - _pos);
    }

    size_t readBytes(char* buffer, size_t length) {
        if (!_valid || length == 0) return 0;
        size_t to_read = std::min(length, _content.size() - _pos);
        memcpy(buffer, _content.data() + _pos, to_read);
        _pos += to_read;
        return to_read;
    }

    void close() {
        _valid = false;
        _pos = 0;
    }

private:
    std::string _content;
    size_t _pos;
    bool _valid;
};

class MockFatFS {
public:
    void set_file(const std::string& path, const std::string& content) {
        _files[path] = content;
    }

    void remove_file(const std::string& path) {
        _files.erase(path);
    }

    void clear() {
        _files.clear();
    }

    File open(const char* filepath, const char* mode = "r") {
        (void)mode;
        if (!filepath) return File();
        auto it = _files.find(filepath);
        if (it != _files.end()) {
            return File(it->second);
        }
        return File();
    }

    bool exists(const char* filepath) {
        if (!filepath) return false;
        return _files.find(filepath) != _files.end();
    }

private:
    std::unordered_map<std::string, std::string> _files;
};

extern MockFatFS FatFS;
