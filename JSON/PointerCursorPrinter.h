#pragma once

#include "constants.h"
#include "macros.h"

#include <cstring>

// ============================================================
//  PointerCursorPrinter
// ============================================================

NAMESPACE_JSON_BEGIN

class PointerCursorPrinter {
public:
    explicit constexpr PointerCursorPrinter(): _pos(0) {}

    // flush():
    // Empty implementation by default in Print::
    // should wait for all outgoing characters to be sent, output buffer is empty after this call
    void flush() { }

    // by default write timeout is possible (outgoing data from network,serial..)
    // (children can override to false (like String))
    bool outputCanTimeout () { return true; }

    // default to zero, meaning "a single write may block"
    // should be overridden by subclasses with buffering
    int availableForWrite() { return UINT32_MAX; }

    // Avance le pointeur de n octets
    void advance(size_t n = 1) { _pos += n; }

    size_t available() { return UINT32_MAX; }
    // Ecrit et avance
    size_t write(uint8_t c) {
        std::printf("%c", (const char)c);        
        return 1;
    }

    size_t write(const uint8_t *buf, size_t size) {
        std::printf("%.*s", (int)size, (const char *)buf);
        return size;
    }

    size_t write(const char *buf, size_t size) {
        std::printf("%.*s", (int)size, buf);
        return size;
    }

    size_t write(const char *buf) {
        return write((const uint8_t *)buf, strlen(buf));
    }

    template<size_t N>
    size_t write(const char (&buf)[N]) {
        return write(buf, N - 1);
    }

    template<size_t N>
    size_t write(char (&buf)[N]) {
        return write(buf, N - 1);
    }

    template <typename... Args>
    std::enable_if_t<(sizeof...(Args) > 0), size_t>
    printf(const char *format, Args &&...args) {
        char buffer[JSON::MAX_PRINTF_BUFFER_SIZE];
        size_t len = snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);
        return write(buffer, len);
    }
    
    bool eof() const { return false; }

    size_t bytesConsumed() const { return _pos; }

    // Avance jusqu'au pointeur
    void advance_to(char *end) {
    }

    void advance_to(size_t pos) {
    }

    size_t size() {
        return UINT32_MAX;
    }

private:
    size_t _pos;
};

NAMESPACE_JSON_END