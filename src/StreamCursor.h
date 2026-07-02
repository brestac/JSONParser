#pragma once

#include <cstdlib>
#include <cstring>

#ifdef ARDUINO
#include "Stream.h"
#else
#include "../include/Stream.h"
#endif

#include "StringPool.h"
#include "constants.h"
#include "demangled.h"
#include "macros.h"

NAMESPACE_JSON_BEGIN

// ============================================================
// RingBuffer<N>
// Buffer circulaire de taille N (doit être une puissance de 2).
// Se remplit à la demande depuis un Stream Arduino.
// ============================================================

template <size_t N> class RingBuffer {
  static_assert(N >= 16, "RingBuffer : N doit être >= 16");
  static_assert((N & (N - 1)) == 0,
                "RingBuffer : N doit être une puissance de 2");

public:
  explicit RingBuffer(Stream *stream) : _stream(stream), _head(0), _tail(0) {}

  // Nombre d'octets disponibles en lecture sans refill
  size_t available() const { return _head - _tail; }

  // Tente de remplir le buffer depuis le stream (appels non-bloquants).
  // Ne lit que les octets immédiatement disponibles.
  void refill() {
    size_t space = N - available();
    if (space == 0) return;

    // Sur WiFiClient, available() peut retourner 0 entre deux paquets
    // alors que le stream n'est pas terminé. On se limite à ce qui est
    // réellement disponible, ou 1 si rien n'est dispo (pour déclencher
    // un timedRead() qui attend le prochain octet).
    size_t hint = (size_t)_stream->available();
    if (hint == 0) hint = 1;              // force un readBytes bloquant minimal
    size_t to_read = std::min(space, hint);

    size_t head_pos = _head & MASK;
    size_t contiguous = N - head_pos;
    size_t first = std::min(to_read, contiguous);
    size_t n = _stream->readBytes(_buf + head_pos, first);
    _head += n;

    if (n == first && first < to_read) {  // wrap-around
        size_t second = to_read - first;
        n = _stream->readBytes(_buf, second);
        _head += n;
    }
  }
  // Peek à l'offset i (0 = prochain octet), sans consommer.
  // Effectue un refill si nécessaire.
  // Retourne -1 si la donnée n'est pas disponible (timeout / fin de flux).
  int peek(size_t offset = 0) {
    if (offset >= available()){
      refill();
    }

    if (offset >= available()){
      return -1;
    }

    return _buf[(_tail + offset) & MASK];
  }

  // Lit et consomme un octet. Retourne -1 si vide.
  int read() {
    if (available() == 0) {
      refill();
    }

    if (available() == 0) {
      return -1; // vrai timeout/EOF
    }

    return _buf[_tail++ & MASK];
  }

  // Consomme n octets (les marque comme lus)
  void consume(size_t n) { _tail += n; }

private:
  static constexpr size_t MASK = N - 1;

  Stream *_stream;
  uint8_t _buf[N];
  size_t _head; // indice d'écriture absolu
  size_t _tail; // indice de lecture absolu
};

// ============================================================
// StreamCursor<N>
// Curseur de lecture ET d'écriture sur un Stream Arduino.
//
// Côté lecture  : RingBuffer non-bloquant, identique à l'existant.
// Côté écriture : satisfait le concept is_cursor_writer_v utilisé
//                 par JSON::print() et JSONObject::toJSON().
//                 Requiert deux méthodes :
//                   size_t write(const char*)
//                   size_t printf(const char* format, ...)
// ============================================================

enum StreamCursorType : uint8_t { READER = 0, WRITER = 1 };

// Base commune (pas de ring buffer)
class StreamCursorBase {
public:
    StreamCursorBase(Stream* s) : _stream(s), _written(0) {}
    void flush() { _stream->flush(); }
    mutable int8_t depth = -1;
protected:
    Stream* _stream;
    size_t  _written;
};

template<StreamCursorType T>
class StreamCursor;
// Spécialisation WRITER : pas de RingBuffer
template<>
class StreamCursor<StreamCursorType::WRITER> : public StreamCursorBase {
public:
    StreamCursor(Stream* s) : StreamCursorBase(s) {}
    StreamCursor(Stream& s) : StreamCursor(&s) {}
    ~StreamCursor() {}

    size_t write(uint8_t c);
    size_t write(const uint8_t* buf, size_t size);
    template<size_t N> size_t write(const char (&str)[N]);
    size_t write(const char* str);
    template<typename... Args> size_t printf(const char* fmt, Args&&... args);
    int availableForWrite();
    bool outputCanTimeout();
    size_t bytesWritten() const { return _written; }
    bool eof() const { return false; }
    size_t bytesConsumed() const { return 0; }
};

// Spécialisation READER : avec RingBuffer
template<>
class StreamCursor<StreamCursorType::READER> : public StreamCursorBase {
public:
    StreamCursor(Stream* s)
        : StreamCursorBase(s), _ring(s), _consumed(0), _eof(false) {}
    StreamCursor(Stream& s) : StreamCursor(&s) {}
    ~StreamCursor() {}

    int peek(size_t offset = 0);
    size_t peekToken(char* out, size_t maxLen);
    void advance(size_t n = 1);
    int read();
    size_t readBytes(uint8_t* buf, size_t len);
    bool eof() const { return _eof; }
    size_t bytesConsumed() const { return _consumed; }

private:
    RingBuffer<JSON::RING_BUFFER_SIZE> _ring;
    size_t _consumed;
    bool   _eof;
};
/*
template <StreamCursorType T>
class StreamCursor {
public:
  StreamCursor(Stream *stream)
      : _ring(stream), _stream(stream), _consumed(0), _written(0), _eof(false) {
    JSON_DEBUG_TYPES("StreamCursor created from %s\n", stream);
  }

  StreamCursor(Stream &stream) : StreamCursor(&stream) {}

  ~StreamCursor() { JSON_DEBUG_WARNING("StreamCursor destroyed\n"); }

  // --------------------------------------------------------
  // Méthodes de LECTURE
  // --------------------------------------------------------
  int peek(size_t offset = 0);
  size_t peekToken(char *out, size_t maxLen);
  void advance(size_t n = 1);
  int read();

// --------------------------------------------------------
// Méthodes d'ÉCRITURE
// --------------------------------------------------------
  size_t write(uint8_t c);
  size_t write(const uint8_t *buffer, size_t size);
  template <size_t N> size_t write(const char (&str)[N]);
  size_t write(const char *str);
  template <typename... Args> size_t printf(const char *format, Args &&...args);
  bool outputCanTimeout();
  int availableForWrite();
  size_t bytesWritten() const;

// --------------------------------------------------------
// Méthodes communes
// --------------------------------------------------------
  bool eof() const { return _eof; }
  size_t bytesConsumed() const { return _consumed; }
  void flush() { _stream->flush(); }

  mutable int8_t depth = -1;

private:
  RingBuffer<JSON::RING_BUFFER_SIZE> _ring;
  Stream *_stream; // référence directe pour l'écriture
  size_t _consumed;
  size_t _written;
  bool _eof;
};
*/
using StreamCursorReader = StreamCursor<StreamCursorType::READER>;
using StreamCursorWriter = StreamCursor<StreamCursorType::WRITER>;
// --------------------------------------------------------
// IMPLEMENTATION DES Méthodes de Lecture
// --------------------------------------------------------

// Caractère courant sans avancer (-1 = fin de flux)
int StreamCursorReader::peek(size_t offset) {
  int c = _ring.peek(offset);
  if (c < 0)
    _eof = true;
  return c;
}

// Avance d'un cran
void StreamCursorReader::advance(size_t n) {
  _ring.consume(n);
  _consumed += n;
}

// Lit et avance d'un cran
int StreamCursorReader::read() {
  int c = _ring.read();
  if (c >= 0)
    _consumed++;
  else
    _eof = true;
  return c;
}

// Extrait au plus maxLen octets dans out[] en s'arrêtant sur un
// délimiteur JSON. Ne consomme PAS les octets (lecture seule via peek).
// Retourne le nombre d'octets copiés.
size_t StreamCursorReader::peekToken(char *out, size_t maxLen) {
  size_t n = 0;
  while (n < maxLen) {
    CHECK_LOOP(MAX_ITERATIONS, 0);
    int c = _ring.peek(n);
    if (c < 0)
      break;
    char ch = static_cast<char>(c);
    bool isDelim = false;
    for (char d : JSON_DELIMITERS) {
      if (ch == d) {
        isDelim = true;
        break;
      }
    }
    if (isDelim)
      break;
    out[n++] = ch;
  }
  if (n < maxLen)
    out[n] = '\0';
  return n;
}

// --------------------------------------------------------
// IMPLEMENTATION DES Méthodes d'écriture
// --------------------------------------------------------
int StreamCursorWriter::availableForWrite() { return _stream->availableForWrite(); }

size_t StreamCursorWriter::write(uint8_t c) {
  // JSON_DEBUG_WARNING("\nStreamCursor::write n=1\n");
  int available = availableForWrite();
  if (available <= 0)
    return 0;

  flush();
  size_t n = _stream->write(c);
  _written += n;

  return n;
}

size_t StreamCursorWriter::write(const uint8_t *buffer, size_t size) {

  size_t len = static_cast<size_t>(availableForWrite());

  if (len > size)
    len = size;

  if (len == 0)
    return 0;

  flush();
  // DEBUG_PRINTF("\nStreamCursor::write n=%zu\n", (size_t)len);

  size_t n = _stream->write(buffer, len);
  _written += n;

  return n;
}
// Écrit une chaîne null-terminée dans le stream.
// Retourne le nombre d'octets écrits.
template <size_t N> size_t StreamCursorWriter::write(const char (&str)[N]) {
  return write((const uint8_t *)str, N);
}

size_t StreamCursorWriter::write(const char *str) {
  return write((const uint8_t *)str, strlen(str));
}

// Écrit une chaîne formatée (printf-style) dans le stream.
// Utilise un buffer de pile de 64 octets ; alloue dynamiquement
// si la chaîne formatée est plus longue.
// Retourne le nombre d'octets écrits.

template <typename... Args>
size_t StreamCursorWriter::printf(const char *format, Args &&...args) {
  char buf[STREAM_BUFFER_SIZE];

  // snprintf écrit au plus sizeof(buf)-1 caractères
  int needed =
      snprintf(buf, sizeof(buf), format, std::forward<Args>(args)...);
  if (needed < 0)
    return 0;

  int available = availableForWrite();
  size_t to_write = static_cast<size_t>(needed);

  if (to_write >= sizeof(buf)) {
    // Le buffer était trop petit : allocation dynamique
    char *heap = static_cast<char *>(malloc(to_write + 1));
    if (!heap)
      return 0;
    snprintf(heap, to_write + 1, format, std::forward<Args>(args)...);
    if (available >= 0 && to_write > static_cast<size_t>(available)) {
      to_write = static_cast<size_t>(available);
    }

    size_t n = write(reinterpret_cast<const uint8_t *>(heap), to_write);
    free(heap);
    _written += n;

    return n;
  }

  // Tout tient dans le buffer de pile
  if (available >= 0 && to_write > static_cast<size_t>(available)) {
    to_write = static_cast<size_t>(available);
  }

  size_t n = write(reinterpret_cast<const uint8_t *>(buf), to_write);
  _written += n;

  return n;
}

bool StreamCursorWriter::outputCanTimeout() { return _stream->outputCanTimeout(); }

NAMESPACE_JSON_END
