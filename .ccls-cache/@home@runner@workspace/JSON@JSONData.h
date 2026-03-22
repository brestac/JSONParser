#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "PointerCursorWriter.h"
#include "PointerCursorPrinter.h"

#ifdef ARDUINO
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

////////////////////////////////////////////////////////////////////////////////
//  toJSON
////////////////////////////////////////////////////////////////////////////////
size_t JSONData::toJSON(bool updates) {
  PointerCursorPrinter writer;
  return toJSON(writer, updates);
}

size_t JSONData::toJSON(char *output, size_t size, bool updates) {
  PointerCursorWriter writer(output, size);
  return toJSON(writer, updates);
}

void JSONData::clearUpdated() { updated = 0; }
