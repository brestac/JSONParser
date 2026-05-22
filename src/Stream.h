#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

class Stream {
public:
  // Writing
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size) = 0;
  virtual void flush() {}
  virtual bool outputCanTimeout() { return true; }
  virtual int availableForWrite() { return 0; }

  // Reading
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }

  // virtual ~Stream();
};
