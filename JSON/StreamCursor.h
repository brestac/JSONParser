#pragma once

#include "constants.h"
#include "macros.h"
#include "demangled.h"

#if defined(ARDUINO) && !defined(ARDUINO_EMULATE_STREAM)
#include <Stream.h>
#endif

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
  explicit RingBuffer(Stream &stream) : _stream(stream), _head(0), _tail(0) {}

  // Nombre d'octets disponibles en lecture sans refill
  size_t available() const { return _head - _tail; }

  // Tente de remplir le buffer depuis le stream (appels non-bloquants).
  // Ne lit que les octets immédiatement disponibles.
  void refill() {
    while (available() < N) {
      int avail = _stream.available();
      if (avail <= 0)
        break;
      int c = _stream.read();
      if (c < 0)
        break;
      _buf[_head++ & MASK] = static_cast<char>(c);
    }
  }

  // Peek à l'offset i (0 = prochain octet), sans consommer.
  // Effectue un refill si nécessaire.
  // Retourne -1 si la donnée n'est pas disponible (timeout / fin de flux).
  int peek(size_t offset = 0) {
    if (offset >= available())
      refill();
    if (offset >= available())
      return -1;
    return static_cast<unsigned char>(_buf[(_tail + offset) & MASK]);
  }

  // Consomme n octets (les marque comme lus)
  void consume(size_t n) { _tail += n; }

  // Lit et consomme un octet. Retourne -1 si vide.
  int read() {
    if (available() == 0)
      refill();
    if (available() == 0)
      return -1;
    return static_cast<unsigned char>(_buf[_tail++ & MASK]);
  }

private:
  static constexpr size_t MASK = N - 1;

  Stream &_stream;
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
  StreamCursor(Stream &stream)
      : _ring(stream), _stream(stream), _consumed(0), _written(0), _eof(false) {
    JSON_DEBUG_TYPES("StreamCursor created from %s\n", stream);
  }

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
  size_t write(uint8_t c) { return _stream.write(c); }
  // Écrit une chaîne null-terminée dans le stream.
  // Retourne le nombre d'octets écrits.
  size_t write(const char *str) {
    if (!str)
      return 0;

    size_t len = std::min((size_t)_stream.availableForWrite(), strlen(str));
    //size_t len = strlen(str);
      
    size_t n = _stream.write((const uint8_t *)str, len);
    _written += n;
    return n;
  }

  // Écrit une chaîne formatée (printf-style) dans le stream.
  // Utilise un buffer de pile de 64 octets ; alloue dynamiquement
  // si la chaîne formatée est plus longue.
  // Retourne le nombre d'octets écrits.
  size_t printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buf[64];
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(buf, sizeof(buf), format, args);
    // size_t available = _stream.availableForWrite();
    // if (static_cast<int>(available) < needed) {
    //    needed = static_cast<int>(available); 
    // }

    //JSON_DEBUG_INFO("StreamCursor::printf %s available=%zu needed=%d\n", format, available, needed);

    va_end(args);

    if (needed < 0) {
      va_end(args_copy);
      return 0;
    }

    size_t n = 0;
    if (static_cast<size_t>(needed) < sizeof(buf)) {
      // Tout tient dans le buffer de pile
      n = _stream.write((const uint8_t *)buf, needed);
    } else {
      // Allocation dynamique pour les chaînes longues
      char *heap = new (std::nothrow) char[needed + 1];
      if (heap) {
        vsnprintf(heap, needed + 1, format, args_copy);
        n = _stream.write((const uint8_t *)heap, needed);
        delete[] heap;
      }
    }
    va_end(args_copy);

    _written += n;
    return n;
  }

  void flush() { _stream.flush(); }

  bool outputCanTimeout() { return _stream.outputCanTimeout(); }

  // Nombre total d'octets écrits depuis la création du curseur
  size_t bytesWritten() const { return _written; }

private:
  RingBuffer<JSON::RING_BUFFER_SIZE> _ring;
  Stream &_stream; // référence directe pour l'écriture
  size_t _consumed;
  size_t _written;
  bool _eof;
};

NAMESPACE_JSON_END