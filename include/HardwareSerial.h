#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <climits>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>

#include "Stream.h"

#define PRINTF_BUFFER_SIZE 4096

// ============================================================
//  HardwareSerial
// ============================================================

class HardwareSerial : public Stream {
public:
  explicit HardwareSerial() : _pos(0) {}

  // flush():
  // Empty implementation by default in Print::
  // should wait for all outgoing characters to be sent, output buffer is empty
  // after this call
  void flush() override {}

  // by default write timeout is possible (outgoing data from network,serial..)
  // (children can override to false (like String))
  bool outputCanTimeout() override { return true; }

  // default to zero, meaning "a single write may block"
  // should be overridden by subclasses with buffering
  int availableForWrite() override { return INT_MAX; }

  // Avance le pointeur de n octets
  void advance(size_t n = 1) { _pos += n; }

  int available() override { return UINT32_MAX; }
  // Ecrit et avance
  size_t write(uint8_t c) override {
    std::printf("%c", c);
    return 1;
  }

  size_t write(const uint8_t *buf, size_t size) override {
    std::printf("%.*s", (int)size, (const char *)buf);
    return size;
  }

  size_t write(const char *buf, size_t size) {
    return write((const uint8_t *)buf, size);
  }

  size_t write(const char *buf) {
    return write((const uint8_t *)buf, strlen(buf));
  }

  template <size_t N> size_t write(const char (&buf)[N]) {
    return write(buf, N - 1);
  }

  template <size_t N> size_t write(char (&buf)[N]) { return write(buf, N - 1); }

  template <typename... Args>
  std::enable_if_t<(sizeof...(Args) > 0), size_t> printf(const char *format, Args &&...args) {
    char buffer[PRINTF_BUFFER_SIZE];
    size_t len = snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);
    return write(buffer, len);
  }
 
  template<size_t N>
  size_t printf(const char (&buffer)[N]) {
    return write(buffer, N - 1);
  }

template<size_t N>
size_t println(const char (&buffer)[N]) {
  size_t len = write(buffer, N - 1);
  len += write("\n");
  return len;
}

  bool eof() const { return false; }

  size_t bytesConsumed() const { return _pos; }

  size_t size() { return UINT32_MAX; }

private:
  size_t _pos;
};

HardwareSerial Serial;
