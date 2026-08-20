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

  explicit constexpr PointerCursor(const char* start)
      : _pos(start), _start(start), _end(start + str_length(start, MAX_JSON_LENGTH)) {
    JSON_DEBUG_COLOR(COLOR_BLUE, "PointerCursor reader %p created with const char* %p ", this, start);
  }

  explicit constexpr PointerCursor(std::string_view& sv)
      : _pos(sv.data()), _start(sv.data()), _end(sv.data() + sv.length()) {
        JSON_DEBUG_COLOR(COLOR_BLUE,
        "PointerCursor reader created with string_view %p size %zu\n",
        sv.data(), sv.length());
  }

  explicit constexpr PointerCursor(std::string& str)
    : _pos(str.c_str()), _start(str.c_str()), _end(str.c_str() + str.length()) {
      JSON_DEBUG_COLOR(COLOR_BLUE,
      "PointerCursor reader created with std::string %p size %zu\n",
        str, str.length());
}

  template <size_t N>
  explicit constexpr PointerCursor(T (&buffer)[N])
      : _pos(buffer), _start(buffer), _end(buffer + N - 1) {
    [[maybe_unused]] constexpr bool is_reader = std::is_same_v<T, const char>;
    JSON_DEBUG_WARNING(
        "PointerCursor %s created with template buffer %p deducted size %zu\n",
        is_reader ? "reader" : "writer", buffer, N - 1);
  }

public:
/*
  // --------------------------------------------------------
  // Méthodes de LECTURE
  // --------------------------------------------------------
  bool advance(int n = 1) const;
  bool go_to(const char* ptr) const;
  int peek(int offset = 0) const;
  int read() const;

// --------------------------------------------------------
// Méthodes d'ECRITURE
// --------------------------------------------------------
  size_t write(char c) const;
  size_t write(const char* buf, size_t size) const;
  size_t write(const char* buf) const;
  template <size_t N> size_t write(const char (&buf)[N]) const;
  template <size_t N> size_t write(char (&buf)[N]) const;
  template <typename... Args> std::enable_if_t<(sizeof...(Args) > 0), size_t> printf(const char* format, Args &&...args) const;
*/

// --------------------------------------------------------
// Méthodes communes
// --------------------------------------------------------
  T *start() const { return _start; }
  constexpr T *ptr() const { return _pos; }
  size_t available() const { return _end - _pos; }
  void flush() {}
  bool eof() const { return _pos >= _end; }
  size_t bytesConsumed() const { return _pos - _start; }
  size_t size() const { return _end - _start; }

  mutable T *_pos;
  T *_start;
  T *_end;
  mutable uint8_t depth = 0;
};

class PointerCursorReader : public PointerCursor<const char> {
public:
  using PointerCursor<const char>::PointerCursor;
  // --------------------------------------------------------
  // Méthodes de LECTURE
  // --------------------------------------------------------
  bool advance() const;
  bool advance(int n) const;
  int peek() const;
  int read() const;
  bool go_to(const char* ptr) const;

  PointerCursorReader() = delete;
  PointerCursorReader(const PointerCursorReader &) = delete;
  PointerCursorReader &operator=(const PointerCursorReader &) = delete;
  PointerCursorReader(PointerCursorReader &&) = delete;
  PointerCursorReader &operator=(PointerCursorReader &&) = delete;
  ~PointerCursorReader() = default;

};

class PointerCursorWriter : public PointerCursor<char> {
public:
  using PointerCursor<char>::PointerCursor;
  // --------------------------------------------------------
  // Méthodes d'ECRITURE
  // --------------------------------------------------------
  size_t write(char c) const;
  size_t write(const char* buf, size_t size) const;
  size_t write(const char* buf) const;
  template <size_t N> size_t write(const char (&buf)[N]) const;
  template <size_t N> size_t write(char (&buf)[N]) const;
  template <typename... Args> std::enable_if_t<(sizeof...(Args) > 0), size_t> printf(const char* format, Args &&...args) const;
};
//-------------------------------------------------------------------//
//  Spécialisation pour const char* (lecture seule)
//-------------------------------------------------------------------//

// Avance le pointeur de n octets
bool PointerCursorReader::advance(int n) const {
  if (_pos + n < _start || _pos + n > _end) {
    JSON_DEBUG_WARNING("PointerCursor::advance: under/overflow\n");
    return false;
  }

  _pos += n;
  return true;
}

bool PointerCursorReader::advance() const {
  if (_pos >= _end) {
    return false;
  }
  _pos++;

  return true;
}

// Avance jusqu'au pointeur
bool PointerCursorReader::go_to(const char* ptr) const {
  if (ptr >= _start && ptr < _end) {
    _pos = ptr;
    return true;
  }

  return false;
}

// Caractère courant sans avancer (-1 = fin)
int PointerCursorReader::peek() const {
  if (_pos >= _end)
    return -1;

  return static_cast<unsigned char>(*_pos);
}

// Lit et avance
int PointerCursorReader::read() const {
  if (_pos >= _end)
    return -1;

  return static_cast<unsigned char>(*_pos++);
}

//-------------------------------------------------------------------//
//  Spécialisation pour char* (écriture seule)
//-------------------------------------------------------------------//
// Ecrit et avance
size_t PointerCursorWriter::write(char c) const {
  if (_pos >= _end)
    return 0;
  *_pos = c;
  _pos++;
  return 1;
}

size_t PointerCursorWriter::write(const char* buf, size_t size) const {
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

size_t PointerCursorWriter::write(const char* buf) const { return write(buf, str_length(buf, MAX_JSON_LENGTH)); }

template <size_t N> size_t PointerCursorWriter::write(const char (&buf)[N]) const {
  return write(buf, N - 1);
}

template <size_t N> size_t PointerCursorWriter::write(char (&buf)[N]) const {
  return write(buf, N - 1);
}

template <typename... Args> 
std::enable_if_t<(sizeof...(Args) > 0), size_t> PointerCursorWriter::printf(const char* format,
                                                       Args &&...args) const {
  size_t len =
      snprintf(_pos, available(), format, std::forward<Args>(args)...);
  _pos += len;
  return len;
}

NAMESPACE_JSON_END
