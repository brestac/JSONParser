#pragma once

#include "constants.h"
#include "macros.h"
#include "str_length.h"
// ============================================================
//  PointerCursor
//  Wrapper autour de const char*& pour exposer la même interface
//  que StreamCursor — permet de templatiser JSONParserBase sur
//  le type de curseur sans changer la logique du parser.
//
//  _pos is mutable: a PointerCursor is a position-tracking iterator
//  (like std::istream_iterator). Advancing the position is logically
//  non-mutating on the cursor object, so all movement methods are
//  marked const. This allows JSONParserBase<const PointerCursor<T>>
//  to compile cleanly regardless of how the cursor was passed.
// ============================================================

NAMESPACE_JSON_BEGIN

template <typename T> class PointerCursor {
public:
  explicit constexpr PointerCursor(T *start, size_t len)
      : _pos(start), _start(start), _end(start + len) {
    JSON_DEBUG_COLOR(COLOR_BLUE, "PointerCursor reader %p created with buffer %p "
                       "explicit size %zu\n",
                       this, start, len);
  }

  constexpr PointerCursor(std::string_view& sv)
      : _pos(sv.data()), _start(sv.data()), _end(sv.data() + sv.length()) {
        JSON_DEBUG_COLOR(COLOR_BLUE,
        "PointerCursor reader created with string_view %p size %zu\n",
        sv.data(), sv.length());
  }

  constexpr PointerCursor(const char *buffer)
      : _pos(buffer), _start(buffer), _end(buffer + str_length(buffer, MAX_POINTER_CURSOR_SIZE)) {
      JSON_DEBUG_COLOR(COLOR_BLUE, "PointerCursor reader created with const char* buffer %p deducted size %zu\n", buffer, str_length(buffer, MAX_POINTER_CURSOR_SIZE));
  }

  template <size_t N>
  constexpr PointerCursor(T (&buffer)[N])
      : _pos(buffer), _start(buffer), _end(buffer + N - 1) {
    [[maybe_unused]] constexpr bool is_reader = std::is_same_v<T, const char>;
    JSON_DEBUG_WARNING(
        "PointerCursor %s created with template buffer %p deducted size %zu\n",
        is_reader ? "reader" : "writer", buffer, N - 1);
  }

  // Accès direct au pointeur brut (pour strtod/strtol)
  constexpr T *ptr() const { return _pos; }

  size_t available() const { return _end - _pos; }

  void flush() {}

  // Avance le pointeur de n octets
  void advance(size_t n = 1) const {
    if (_pos + n <= _end) {
      _pos += n;
    } else {
      _pos = _end;
      JSON_DEBUG_WARNING("PointerCursor::advance: overflow\n");
    }
  }

// Avance jusqu'au pointeur
void advance_to(const char *ptr) const {
  if (ptr >= _start && ptr < _end)
    _pos = ptr;
}

// Avance ou recule jusqu'à la position depuis le debut
void advance_to(size_t index) const {
  if (index < size())
    _pos = _start + index;
}

  // Caractère courant sans avancer (-1 = fin)
  int peek(size_t offset = 0) const {
    const char *p = _pos + offset;
    if (p >= _end)
      return -1;
    return static_cast<unsigned char>(*p);
  }

  // Lit et avance
  int read() const {
    if (_pos >= _end)
      return -1;
    return static_cast<unsigned char>(*_pos++);
  }

  // Ecrit et avance
  size_t write(char c) const {
    if (_pos >= _end)
      return 0;
    *_pos = c;
    _pos++;
    return 1;
  }

  size_t write(const char *buf, size_t size) const {
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

  size_t write(const char *buf) const { return write(buf, str_length(buf, MAX_POINTER_CURSOR_SIZE)); }

  template <size_t N> size_t write(const char (&buf)[N]) const {
    return write(buf, N - 1);
  }

  template <size_t N> size_t write(char (&buf)[N]) const {
    return write(buf, N - 1);
  }

  template <typename... Args>
  std::enable_if_t<(sizeof...(Args) > 0), size_t> printf(const char *format,
                                                         Args &&...args) const {
    size_t len =
        snprintf(_pos, available(), format, std::forward<Args>(args)...);
    _pos += len;
    return len;
  }

  bool eof() const { return _pos >= _end; }

  size_t bytesConsumed() const { return _pos - _start; }

  size_t size() const { return _end - _start; }

  T *start() const { return _start; }

private:
  mutable T *_pos;
  T *_start;
  T *_end;
};

using PointerCursorReader = PointerCursor<const char>;
using PointerCursorWriter = PointerCursor<char>;

NAMESPACE_JSON_END
