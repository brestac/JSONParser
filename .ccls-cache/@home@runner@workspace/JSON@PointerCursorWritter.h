#pragma once

#include "constants.h"
#include "macros.h"

#include <stddef.h>
#include <stdint.h>
// ============================================================
//  PointerCursor
//  Wrapper autour de char*& pour exposer la même interface
//  que StreamCursor — permet de templatiser JSONParserBase sur
//  le type de curseur sans changer la logique du parser.
// ============================================================


NAMESPACE_JSON_BEGIN

class PointerCursorWritter {
public:
    explicit constexpr PointerCursorWritter(char *start, size_t len): _pos(start), _start(start), _end(start + len) {}

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

    size_t write(const uint8_t *buf, size_t size) {
        if (_pos + size >= _end) return 0;

        size_t i = 0;
        for(; i < size; i++) {
            *_pos = buf[i];
            _pos++;
        }

        return size;
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
    
    constexpr PointerCursorWritter() : PointerCursorWritter((char *)nullptr, 0) {}
    constexpr PointerCursorWritter(std::string_view sv) : PointerCursorWritter(sv.data(), sv.length()) {} 
    template <size_t N> constexpr PointerCursorWritter(char (&buffer)[N]) : PointerCursorWritter(buffer, N - 1) {}
    template <size_t N> constexpr PointerCursorWritter(char (buffer)[N]) : PointerCursorWritter(buffer, N - 1) {}

private:
    char *_pos;
    char *_start;
    char *_end;
};

NAMESPACE_JSON_END