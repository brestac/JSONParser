#pragma once

#include <cstdlib>
#include <cstring>

#ifdef ARDUINO
#include "Stream.h"
#else
#include "../include/Stream.h"
#endif

#include "StaticString.h"
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

  // Écriture dans la partie libre du ring buffer (contiguë modulo N)
  // On lit par blocs dans un tmp puis on copie octet par octet
  static char tmp[N];
  size_t n = _stream->readBytes(tmp, space);  // bloquant avec timeout
  for (size_t i = 0; i < n; i++) {
      _buf[_head & MASK] = tmp[i];
      _head++;
  }
}
// Peek à l'offset i (0 = prochain octet), sans consommer.
// Effectue un refill si nécessaire.
// Retourne -1 si la donnée n'est pas disponible (timeout / fin de flux).
int peek(size_t offset = 0) {
  if (offset >= available()) refill();
  if (offset >= available()) return -1;
  return static_cast<unsigned char>(_buf[(_tail + offset) & MASK]);
}

// Lit et consomme un octet. Retourne -1 si vide.
int read() {
  if (!_stream) return -1;
  if (available() == 0) refill();
  if (available() == 0) return -1;  // vrai timeout/EOF
  return static_cast<unsigned char>(_buf[_tail++ & MASK]);
}


  // Consomme n octets (les marque comme lus)
  void consume(size_t n) { _tail += n; }

private:
  static constexpr size_t MASK = N - 1;

  Stream *_stream;
  char _buf[N];
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

class StreamCursor {
public:

  StreamCursor(Stream* stream) : _ring(stream), _stream(stream), _consumed(0), _written(0), _eof(false) {
    JSON_DEBUG_TYPES("StreamCursor created from %s\n", stream);
  }

  StreamCursor(Stream& stream) : StreamCursor(&stream) { }

  ~StreamCursor() { JSON_DEBUG_WARNING("StreamCursor destroyed\n"); }

  // --------------------------------------------------------
  // Méthodes de LECTURE (existantes, inchangées)
  // --------------------------------------------------------

  // Caractère courant sans avancer (-1 = fin de flux)
  int peek(size_t offset = 0) {
    int c = _ring.peek(offset);
    if (c < 0)
      _eof = true;
    return c;
  }

  // Avance d'un cran
  void advance(size_t n = 1) {
    _ring.consume(n);
    _consumed += n;
  }

  // Lit et avance d'un cran
  int read() {
    int c = _ring.read();
    if (c >= 0)
      _consumed++;
    else
      _eof = true;
    return c;
  }

  bool eof() const { return _eof; }

  // Nombre total d'octets consommés depuis la création du curseur
  size_t bytesConsumed() const { return _consumed; }

  // Extrait au plus maxLen octets dans out[] en s'arrêtant sur un
  // délimiteur JSON. Ne consomme PAS les octets (lecture seule via peek).
  // Retourne le nombre d'octets copiés.
  size_t peekToken(char *out, size_t maxLen) {
    static const char delimiters[] = {',',  '}',  ']',  ' ',
                                      '\t', '\n', '\r', '\0'};
    size_t n = 0;
    while (n < maxLen) {
      CHECK_LOOP(MAX_ITERATIONS, 0);
      int c = _ring.peek(n);
      if (c < 0)
        break;
      char ch = static_cast<char>(c);
      bool isDelim = false;
      for (char d : delimiters) {
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
  // Méthodes d'ÉCRITURE
  // Satisfont le concept is_cursor_writer_v de JSONPrinter.h
  // --------------------------------------------------------

  /*
  print.h overload
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size);
  virtual void flush() { }
  virtual bool outputCanTimeout () { return true; }
  virtual int availableForWrite() { return 0; }
  */
  int availableForWrite() { return _stream->availableForWrite(); }

  size_t write(uint8_t c) {
    // JSON_DEBUG_WARNING("\nStreamCursor::write n=1\n");
    int available = availableForWrite();
    if (available <= 0)
      return 0;

    flush();
    size_t n = _stream->write(c);
    _written += n;

    return n;
  }

  size_t write(const uint8_t *buffer, size_t size) {

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
  template <size_t N> size_t write(const char (&str)[N]) {
    return write((const uint8_t *)str, N);
  }

  size_t write(const char* str) {
    return write((const uint8_t *)str, strlen(str));
  }

  // Écrit une chaîne formatée (printf-style) dans le stream.
  // Utilise un buffer de pile de 64 octets ; alloue dynamiquement
  // si la chaîne formatée est plus longue.
  // Retourne le nombre d'octets écrits.

  template <typename... Args>
  size_t printf(const char* format, Args &&...args) {
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

  void flush() { _stream->flush(); }

  bool outputCanTimeout() { return _stream->outputCanTimeout(); }

  // Nombre total d'octets écrits depuis la création du curseur
  size_t bytesWritten() const { return _written; }

  int8_t depth = -1;
private:
  RingBuffer<JSON::RING_BUFFER_SIZE> _ring;
  Stream *_stream; // référence directe pour l'écriture
  size_t _consumed;
  size_t _written;
  bool _eof;
};

NAMESPACE_JSON_END