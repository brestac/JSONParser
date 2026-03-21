#pragma once

#include "constants.h"
#include "macros.h"

#include <stddef.h>
#include <stdint.h>
#include <cstring>
// ============================================================
//  PointerCursor
//  Wrapper autour de char*& pour exposer la même interface
//  que StreamCursor — permet de templatiser JSONParserBase sur
//  le type de curseur sans changer la logique du parser.
// ============================================================


NAMESPACE_JSON_BEGIN

class PointerCursorWriter {
public:
    explicit constexpr PointerCursorWriter(char *start, size_t len): _pos(start), _start(start), _end(start + len) {}

    // Accès direct au pointeur brut (pour strtod/strtol)
    char *ptr() const { return _pos; }

    // Avance le pointeur de n octets
    void advance(size_t n = 1) { _pos += n; }

    size_t available() { return _end - _pos; }
    // Ecrit et avance
    size_t write(uint8_t c) {
        if (_pos >= _end) return 0;
        *_pos = c;
        _pos++;
        
        return 1;
    }

    size_t write(const char *buf) {
        return write(buf, strlen(buf));
    }

    size_t write(const char *buf, size_t size) {
        if (_pos >= _end) return 0;

        size_t i = 0;
        for(; i < size; i++) {
            *_pos = buf[i];
            _pos++;
            if (_pos >= _end) break;
        }

        return i;
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
    write(const char *format, Args &&...args) {
        size_t len = snprintf(_pos, available(), format, std::forward<Args>(args)...);
        _pos += len;
        return len;
    }

    bool eof() const { return _pos >= _end; }

    size_t bytesConsumed() const { return _pos - _start; }

    // Avance jusqu'au pointeur
    void advance_to(char *end) {
        if (end > _pos) _pos = end;
    }

    void advance_to(size_t pos) {
        if (pos >= 0) _pos = _start + pos;
    }

    size_t size() {
        return _end - _start;
    }

    const char *start() {
        return _start;
    }
    
    constexpr PointerCursorWriter() : PointerCursorWriter((char *)nullptr, 0) {}
    constexpr PointerCursorWriter(std::string_view sv) : PointerCursorWriter((char *)sv.data(), sv.length()) {} 
    template <size_t N> constexpr PointerCursorWriter(char (&buffer)[N]) : PointerCursorWriter(buffer, N - 1) {}
    template <size_t N> constexpr PointerCursorWriter(char (buffer)[N]) : PointerCursorWriter(buffer, N - 1) {}

private:
    char *_pos;
    char *_start;
    char *_end;
};

NAMESPACE_JSON_END