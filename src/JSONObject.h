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

  ////////////////////////////////////////////////////////////////////////////////
  //  fromJSON
  ////////////////////////////////////////////////////////////////////////////////

  virtual JSON::ParseResult fromJSON(const PointerCursorReader &cursor) { return ParseResult(); }

  virtual JSON::ParseResult fromJSON(StreamCursor &cursor) { return ParseResult(); }

  template <size_t N> ParseResult fromJSON(const char (&input)[N], bool updates) {
    const PointerCursorReader cursor(input, N - 1);
    return fromJSON(cursor);
  }

  JSON::ParseResult fromJSON(const char *input, bool updates) {
    const PointerCursorReader cursor(input, str_length(input));
    return fromJSON(cursor);
  }

  ////////////////////////////////////////////////////////////////////////////////
  //  toJSON
  ////////////////////////////////////////////////////////////////////////////////
  virtual size_t toJSON(PointerCursorWriter &cursor, bool updates = true) { return 0; }

  virtual size_t toJSON(StreamCursor &cursor, bool updates = true) { return 0; }

  void clearUpdated() { updated = 0; }
};
