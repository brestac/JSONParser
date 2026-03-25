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

template <typename T>
class PointerCursor {
public:
  explicit constexpr PointerCursor(T *start, size_t len)
      : _pos(start), _start(start), _end(start + len) {}

  // Accès direct au pointeur brut (pour strtod/strtol)
  constexpr T *ptr() const { return _pos; }

  size_t available() { return _end - _pos; }

  // Avance le pointeur de n octets
  void advance(size_t n = 1) { _pos += n; }

  // Caractère courant sans avancer (-1 = fin)
  int peek(size_t offset = 0) const {
    const char *p = _pos + offset;
    if (p >= _end)
      return -1;
    return static_cast<unsigned char>(*p);
  }

  // Lit et avance
  int read() {
    if (_pos >= _end)
      return -1;
    return static_cast<unsigned char>(*_pos++);
  }
  // Ecrit et avance
  size_t write(char c) {
    if (_pos >= _end)
      return 0;
    *_pos = c;
    _pos++;

    return 1;
  }

  size_t write(const char *buf, size_t size) {
    if (_pos >= _end)
      return 0;

    size_t i = 0;
    for (; i < size; i++) {
      *_pos = buf[i];
      _pos++;
      if (_pos >= _end)
        break;
    }

    return i;
  }

  size_t write(const char *buf) { return write(buf, str_length(buf)); }

  template <size_t N> size_t write(const char (&buf)[N]) {
    return write(buf, N - 1);
  }

  template <size_t N> size_t write(char (&buf)[N]) { return write(buf, N - 1); }

  template <typename... Args>
  std::enable_if_t<(sizeof...(Args) > 0), size_t> printf(const char *format,
                                                         Args &&...args) {
    size_t len =
        snprintf(_pos, available(), format, std::forward<Args>(args)...);
    _pos += len;
    return len;
  }

  bool eof() const { return _pos >= _end; }

  size_t bytesConsumed() const { return _pos - _start; }

  // Avance jusqu'au pointeur
  void advance_to(const char *end) {
    if (end > _pos)
      _pos = end;
  }

  void set_position(size_t pos) {
    if (pos >= 0)
      _pos = _start + pos;
  }

  size_t size() { return _end - _start; }

  T *start() { return _start; }

  constexpr PointerCursor()
      : PointerCursor(static_cast<const char *>(nullptr),
                      static_cast<size_t>(0)) {}
  // constexpr PointerCursor(char *buffer, size_t size)
  //     : PointerCursor(static_cast<const char *>(buffer), size) {}
  constexpr PointerCursor(const char *buffer)
      : PointerCursor(buffer, str_length(buffer)) {}
  constexpr PointerCursor(std::string_view sv)
      : PointerCursor(sv.data(), sv.length()) {}
  template <size_t N>
  constexpr PointerCursor(char (&buffer)[N]) : PointerCursor(buffer, N - 1) {}
  template <size_t N>
  constexpr PointerCursor(const char(buffer)[N])
      : PointerCursor(buffer, N - 1) {}

private:
  T *_pos;
  T *_start;
  T *_end;
};

using PointerCursorReader = PointerCursor<const char>;
using PointerCursorWriter = PointerCursor<char>;

NAMESPACE_JSON_END