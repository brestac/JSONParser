#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "StreamCursor.h"

using namespace JSON;

struct JSONObject {
public:
  uint32_t updated = 0;

  virtual ~JSONObject() = default;
  virtual JSON::ParseResult fromJSON(const PointerCursorReader& cursor) = 0;
  virtual size_t toJSON(PointerCursorWriter& cursor, bool updates = true) = 0;

  virtual JSON::ParseResult fromJSON(StreamCursor &cursor) = 0;
  virtual size_t toJSON(StreamCursor& cursor, bool updates = true) = 0;

  JSON::ParseResult fromJSON(char *input, size_t size);
  size_t toJSON(char *output, size_t size, bool updates = true);

  void clearUpdated();
};

////////////////////////////////////////////////////////////////////////////////
//  fromJSON
////////////////////////////////////////////////////////////////////////////////
JSON::ParseResult JSONObject::fromJSON(char *input, size_t size) {
  const PointerCursorReader cursor(input, size);
  return fromJSON(cursor);
}

////////////////////////////////////////////////////////////////////////////////
//  toJSON
////////////////////////////////////////////////////////////////////////////////

size_t JSONObject::toJSON(char *output, size_t size, bool updates) {
  PointerCursorWriter writer(output, size);
  return toJSON(writer, updates);
}

void JSONObject::clearUpdated() { updated = 0; }
