#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ArduinoCompat.h"

#define PARSE_TIMEOUT 1000 // default number of milli-seconds to wait
#define NO_SKIP_CHAR 1 // a magic char not found in a valid ASCII numeric field

class Stream {
protected:
  unsigned long _timeout = PARSE_TIMEOUT; // number of milliseconds to wait for the next
                                 // char before aborting timed read
  unsigned long _startMillis;    // used for timeout measurement
  int timedRead();               // private method to read stream with timeout
  int timedPeek();               // private method to peek stream with timeout

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
  virtual void setTimeout(size_t timeout) { _timeout = timeout; }
  virtual size_t getTimeout() { return _timeout; }

  size_t readBytes(uint8_t *buffer, size_t length);
  size_t read(uint8_t* buffer, size_t maxLen);
};

// private method to read stream with timeout
int Stream::timedRead() {
  int c;
  _startMillis = millis();
  do {
    c = read();
    if (c >= 0)
      return c;
    if (_timeout == 0)
      return -1;
    yield();
  } while (millis() - _startMillis < _timeout);
  return -1; // -1 indicates timeout
}

// private method to peek stream with timeout
int Stream::timedPeek() {
  int c;
  _startMillis = millis();
  do {
    c = peek();
    if (c >= 0)
      return c;
    if (_timeout == 0)
      return -1;
    yield();
  } while (millis() - _startMillis < _timeout);
  return -1; // -1 indicates timeout
}

// read characters from stream into buffer
// terminates if length characters have been read, or timeout (see setTimeout)
// returns the number of characters placed in the buffer
// the buffer is NOT null terminated.
//
size_t Stream::readBytes(uint8_t *buffer, size_t length) {

  size_t count = 0;
  while (count < length) {
    int c = timedRead();
    if (c < 0)
      break;
    *buffer++ = c;
    count++;
  }
  return count;
}

size_t Stream::read(uint8_t* buffer, size_t maxLen)
{
    size_t nbread = 0;
    while (nbread < maxLen && available())
    {
        int c = read();
        if (c == -1)
            break;
        buffer[nbread++] = c;
    }
    return nbread;
}