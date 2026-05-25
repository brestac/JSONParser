#pragma once
/**
    StreamString.h

    StreamString is a class that implements the Stream interface and allows
    to read and write to a char buffer.

    This is a stub for testing purposes.
    Based on https://github.com/esp8266/Arduino/blob/master/cores/esp8266/StreamString.h
*/

#include "Stream.h"
#include <climits>
#include <cstring>
#include <cstdarg>

class StreamString : public Stream {
public:
    StreamString() : _buffer(nullptr), _size(0), _wpos(0), _rpos(0), _eof(false) {}

    StreamString(const char *str) : _buffer(nullptr), _size(0), _wpos(0), _rpos(0), _eof(false) {
        if (str) {
            _size = strlen(str);
            _wpos = _size;
            _buffer = (char *)malloc(_size + 1);
            if (_buffer) {
                memcpy(_buffer, str, _size + 1);
            }
        }
    }

    ~StreamString() {
        if (_buffer)
            free(_buffer);
        _buffer = nullptr;
        _size = 0;
        _wpos = 0;
        _rpos = 0;
        _eof = false;
    }

    // Writing
    void flush() override { }
    bool outputCanTimeout() override { return false; }
    int availableForWrite() override { return INT_MAX; }

    size_t write(uint8_t c) override {
        if (_wpos + 1 >= _size) {
            _size = _wpos + 17;
            _buffer = (char *)realloc(_buffer, _size);
            if (!_buffer) return 0;
        }
        _buffer[_wpos++] = (char)c;
        _buffer[_wpos] = '\0';
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        if (_wpos + size + 1 > _size) {
            _size = _wpos + size + 17;
            _buffer = (char *)realloc(_buffer, _size);
            if (!_buffer) return 0;
        }
        memcpy(_buffer + _wpos, buffer, size);
        _wpos += size;
        _buffer[_wpos] = '\0';
        return size;
    }

    size_t printf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        int needed = vsnprintf(nullptr, 0, format, args);
        va_end(args);

        if (needed < 0) {
            return 0;
        }

        size_t needed_size = static_cast<size_t>(needed);
        if (_wpos + needed_size + 1 > _size) {
            _size = _wpos + needed_size + 17;
            _buffer = (char *)realloc(_buffer, _size);
            if (!_buffer) {
                return 0;
            }
        }

        va_start(args, format);
        int written = vsnprintf(_buffer + _wpos, needed_size + 1, format, args);
        va_end(args);

        if (written < 0) {
            return 0;
        }

        _wpos += static_cast<size_t>(written);
        return static_cast<size_t>(written);
    }

    // Reading — forward, from _rpos toward _wpos
    int available() override {
        return (int)(_wpos - _rpos);
    }

    int read() override {
        if (_rpos >= _wpos) {
            _eof = true;
            return -1;
        }
        return (unsigned char)_buffer[_rpos++];
    }

    int peek() override {
        if (_rpos >= _wpos) {
            _eof = true;
            return -1;
        }
        return (unsigned char)_buffer[_rpos];
    }

    bool eof() const { return _eof; }

    size_t bytesConsumed() const { return _rpos; }

    size_t size() const { return _wpos; }

    const char *c_str() const {
        return _buffer ? _buffer : "";
    }

    void clear() {
        if (_buffer)
            free(_buffer);
        _buffer = nullptr;
        _size = 0;
        _wpos = 0;
        _rpos = 0;
        _eof = false;
    }

private:
    char *_buffer;
    size_t _size;
    size_t _wpos;   // write cursor: number of bytes written
    size_t _rpos;   // read cursor: next byte to consume
    bool _eof;
};
