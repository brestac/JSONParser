#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "StreamCursor.h"
#include "str_length.h"

using namespace JSON;

struct JSONObject {
public:
  uint32_t updated = 0;

  virtual ~JSONObject() = default;

  virtual JSON::ParseResult fromJSON(const PointerCursorReader& cursor) {
    return JSON::ParseResult();
  }

  virtual JSON::ParseResult fromJSON(StreamCursor& cursor) {
    return JSON::ParseResult();
  }

  virtual size_t toJSON(PointerCursorWriter& cursor, bool updates = true) {
    return 0;
  }

  virtual size_t toJSON(StreamCursor& cursor, bool updates = true) {
    return 0;
  }

  //JSON::ParseResult fromJSON(const char* input, bool updates = true);

  template<size_t N>
  JSON::ParseResult fromJSON(const char (&input)[N], bool updates = true);

  template<size_t N>
  size_t toJSON(char (&input)[N], bool updates = true);

  void clearUpdated();
};

////////////////////////////////////////////////////////////////////////////////
//  fromJSON
////////////////////////////////////////////////////////////////////////////////
// JSON::ParseResult JSONObject::fromJSON(const char *input, bool updates) {
//   const PointerCursorReader cursor(input, str_length(input));
//   return fromJSON(cursor);
// }

template<size_t N>
JSON::ParseResult JSONObject::fromJSON(const char (&input)[N], bool updates) {
  const PointerCursorReader cursor(input, N - 1);
  return fromJSON(cursor);
}

////////////////////////////////////////////////////////////////////////////////
//  toJSON
////////////////////////////////////////////////////////////////////////////////
template<size_t N>
size_t JSONObject::toJSON(char (&output)[N], bool updates) {
  PointerCursorWriter writer(output, N - 1);
  return toJSON(writer, updates);
}

void JSONObject::clearUpdated() { updated = 0; }
