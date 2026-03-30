#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"

#ifdef ARDUINO
#include "StreamCursor.h"
#else
#include "PointerCursorPrinter.h"
#endif

using namespace JSON;

struct JSONData {
public:
  uint32_t updated = 0;

  virtual ~JSONData() = default;
  virtual JSON::ParseResult fromJSON(const PointerCursorReader& cursor) = 0;
  virtual size_t toJSON(PointerCursorWriter& cursor, bool updates = true) = 0;

#ifdef ARDUINO
  virtual JSON::ParseResult fromJSON(StreamCursor &cursor) = 0;
  virtual size_t toJSON(StreamCursor& cursor, bool updates = true) = 0;
#else
  virtual size_t toJSON(PointerCursorPrinter& cursor, bool updates = true) = 0;
#endif

  JSON::ParseResult fromJSON(char *input, size_t size);
  size_t toJSON(bool updates = true);
  size_t toJSON(char *output, size_t size, bool updates = true);

  void clearUpdated();
};

////////////////////////////////////////////////////////////////////////////////
//  fromJSON
////////////////////////////////////////////////////////////////////////////////
JSON::ParseResult JSONData::fromJSON(char *input, size_t size) {
  const PointerCursorReader cursor(input, size);
  return fromJSON(cursor);
}

////////////////////////////////////////////////////////////////////////////////
//  toJSON
////////////////////////////////////////////////////////////////////////////////
#ifdef ARDUINO
size_t JSONData::toJSON(bool updates) {
  StreamCursor writer(Serial);
  return toJSON(writer, updates);
}
#else
size_t JSONData::toJSON(bool updates) {
  PointerCursorPrinter writer;
  return toJSON(writer, updates);
}
#endif

size_t JSONData::toJSON(char *output, size_t size, bool updates) {
  PointerCursorWriter writer(output, size);
  return toJSON(writer, updates);
}

void JSONData::clearUpdated() { updated = 0; }
