#pragma once

// Minimal Arduino Stream stub for desktop testing.
// Provides the interface used by JSON::StreamCursor and the JSON macros.

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <new>

// ----------------------------------------------------------------
// Stream — abstract base (mirrors the real Arduino Stream/Print API)
// ----------------------------------------------------------------
class Stream {
public:
    virtual ~Stream() = default;

    virtual int    available() = 0;
    virtual int    read()      = 0;
    virtual int    peek()      = 0;

    virtual void flush() {}
    virtual bool outputCanTimeout() { return true; }
    virtual int availableForWrite() { return 0; }

    // Write methods used by StreamCursor::write() / StreamCursor::printf()
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        for (size_t i = 0; i < size; ++i) n += write(buffer[i]);
        return n;
    }

    virtual size_t print(const char *str) {
        if (!str) return 0;
        size_t n = strlen(str);
        fwrite(str, 1, n, stdout);
        return n;
    }
    virtual size_t print(char c) {
        fputc(c, stdout);
        return 1;
    }

    // printf-style output used by PRINT_FUNC (Serial.printf) in macros.h
    int printf(const char *format, ...) {
        va_list args;
        va_start(args, format);
        int n = vprintf(format, args);
        va_end(args);
        return n;
    }
};

// ----------------------------------------------------------------
// StringStream — readable stream backed by a null-terminated string.
// Writes go to stdout so toJSON output is visible in the terminal.
// ----------------------------------------------------------------
class StringStream : public Stream {
public:
    explicit StringStream(const char *data)
        : _data(data), _pos(0), _len(data ? strlen(data) : 0) {}

    int available() override { return (_pos < _len) ? (int)(_len - _pos) : 0; }
    int read()      override { return (_pos < _len) ? (unsigned char)_data[_pos++] : -1; }
    int peek()      override { return (_pos < _len) ? (unsigned char)_data[_pos]   : -1; }
    void flush()    override {}
    int availableForWrite() override { return available(); }
    bool outputCanTimeout() override { return false; }
    size_t write(uint8_t c) override {
        fputc(c, stdout);
        return 1;
    }
    size_t write(const uint8_t *buffer, size_t size) override {
        return fwrite(buffer, 1, size, stdout);
    }

private:
    const char *_data;
    size_t      _pos;
    size_t      _len;
};

// ----------------------------------------------------------------
// HardwareSerial / Serial — routes prints/printf to stdout.
// ----------------------------------------------------------------

class HardwareSerial : public Stream {
public:
    int    available() override { return 0;  }
    int    read()      override { return -1; }
    int    peek()      override { return -1; }
    void   flush()     override {}
    bool   outputCanTimeout() override { return false; }
    int    availableForWrite() override { return 1024; }
 
    size_t write(uint8_t c) override {
        fputc(c, stdout);
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        fwrite(buffer, 1, size, stdout);
        return size;
    }

    size_t print(const char *str) override {
        if (!str) return 0;
        size_t n = strlen(str);
        fwrite(str, 1, n, stdout);
        return n;
    }

    size_t print(char c) override {
        fputc(c, stdout);
        return 1;
    }
};

inline HardwareSerial Serial;
