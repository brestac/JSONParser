#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "PointerCursorWriter.h"
#include "PointerCursorPrinter.h"

#ifdef ARDUINO
#include <Stream.h>
#include "StreamCursor"
#endif

#include <stddef.h>
#include <stdint.h>

using namespace JSON;

struct JSONData {
public:
  uint32_t updated = 0;

  virtual ~JSONData() = default;

  virtual JSON::ParseResult fromJSON(PointerCursor cursor) = 0;
#ifdef ARDUINO
  virtual JSON::ParseResult fromJSON(StreamCursor cursor) = 0;
#endif
  JSON::ParseResult fromJSON(char *input, size_t size);

  virtual size_t toJSON(PointerCursorWriter cursor, bool updates = true) = 0;
  virtual size_t toJSON(PointerCursorPrinter cursor, bool updates = true) = 0;
#ifdef ARDUINO
  virtual size_t toJSON(StreamCursor cursor, bool updates = true) = 0;
#endif
  size_t toJSON(bool updates = true);
  size_t toJSON(char *output, size_t size, bool updates = true);

  void clearUpdated();
};

////////////////////////////////////////////////////////////////////////////////
//  fromJSON
////////////////////////////////////////////////////////////////////////////////
JSON::ParseResult JSONData::fromJSON(char *input, size_t size) {
  PointerCursor cursor(input, size);
  return fromJSON(cursor);
}

#ifdef ARDUINO
template <typename T>
enable_if_t<std::is_base_of<Stream, T>, JSON::ParseResult> fromJSON(T& cursor) {
  StreamCursor streamCursor(cursor);
  return fromJSON(streamCursor);
}
#endif
////////////////////////////////////////////////////////////////////////////////
//  toJSON
////////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO
template <typename T>
enable_if_t<std::is_base_of<Stream, T>, size_t> toJSON(T& cursor, bool updates = true) {
  StreamCursor streamCursor(cursor);
  return toJSON(streamCursor, updates);
}
#endif

size_t JSONData::toJSON(bool updates) {
  PointerCursorPrinter writer;
  return toJSON(writer, updates);
}

size_t JSONData::toJSON(char *output, size_t size, bool updates) {
  PointerCursorWriter writer(output, size);
  return toJSON(writer, updates);
}

void JSONData::clearUpdated() { updated = 0; }
