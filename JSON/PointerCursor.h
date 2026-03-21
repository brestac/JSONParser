#pragma once

#include "constants.h"
#include "macros.h"
#include "str_length.h"

#include <stddef.h>
// ============================================================
//  PointerCursor
//  Wrapper autour de const char*& pour exposer la même interface
//  que StreamCursor — permet de templatiser JSONParserBase sur
//  le type de curseur sans changer la logique du parser.
// ============================================================


NAMESPACE_JSON_BEGIN

class PointerCursor {
public:
    explicit constexpr PointerCursor(const char *start, size_t len): _pos(start), _start(start), _end(start + len) {}

    // Accès direct au pointeur brut (pour strtod/strtol)
    const char *ptr() const { return _pos; }

    // Avance le pointeur de n octets
    void advance(size_t n = 1) { _pos += n; }

    // Caractère courant sans avancer (-1 = fin)
    int peek(size_t offset = 0) const {
        const char *p = _pos + offset;
        if (p >= _end) return -1;
        return static_cast<unsigned char>(*p);
    }

    // Lit et avance
    int read() {
        if (_pos >= _end) return -1;
        return static_cast<unsigned char>(*_pos++);
    }

    bool eof() const { return _pos >= _end; }

    size_t bytesConsumed() const { return _pos - _start; }

    // Avance jusqu'au pointeur
    void advance_to(const char *end) {
        if (end > _pos) _pos = end;
    }

    void set_position(size_t pos) {
        if (pos >= 0) _pos = _start + pos;
    }

    size_t size() {
        return _end - _start;
    }

    const char *start() {
        return _start;
    }
    
    constexpr PointerCursor() : PointerCursor(static_cast<const char*>(nullptr), static_cast<size_t>(0)) {}
    constexpr PointerCursor(char *buffer, size_t size) : PointerCursor(static_cast<const char*>(buffer), size) {}
    constexpr PointerCursor(const char *buffer) : PointerCursor(buffer, str_length(buffer)) {}
    constexpr PointerCursor(std::string_view sv) : PointerCursor(sv.data(), sv.length()) {} 
    template <size_t N> constexpr PointerCursor(char (&buffer)[N]) : PointerCursor(buffer, N - 1) {}
    template <size_t N> constexpr PointerCursor(const char (buffer)[N]) : PointerCursor(buffer, N - 1) {}

private:
    const char *_pos;
    const char *_start;
    const char *_end;
};

NAMESPACE_JSON_END