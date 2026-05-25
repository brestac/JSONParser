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

  virtual JSON::ParseResult fromJSON(JSON_PARSER_NAME_ARG const PointerCursorReader &cursor) {
    return ParseResult();
  }
  virtual JSON::ParseResult fromJSON(JSON_PARSER_NAME_ARG StreamCursor &cursor) {
    return ParseResult();
  }

  template <size_t N>
  ParseResult fromJSON(JSON_PARSER_NAME_ARG const char (&input)[N], bool updates) {
    JSON_DEBUG_WARNING("JSONObject::fromJSON(const char (&input)[N])\n");
    const PointerCursorReader cursor(input, N - 1);
    return fromJSON(JSON_PARSER_NAME_PASS cursor);
  }

  JSON::ParseResult fromJSON(JSON_PARSER_NAME_ARG const char *input, bool updates) {
    JSON_DEBUG_WARNING("JSONObject::fromJSON(const char *input)\n");
    const PointerCursorReader cursor(input, str_length(input, MAX_POINTER_CURSOR_SIZE));
    return fromJSON(JSON_PARSER_NAME_PASS cursor);
  }

  ////////////////////////////////////////////////////////////////////////////////
  //  toJSON
  ////////////////////////////////////////////////////////////////////////////////
  virtual size_t toJSON(PointerCursorWriter &cursor, bool updates = true) {
    return 0;
  }

  virtual size_t toJSON(StreamCursor &cursor, bool updates = true) { return 0; }

  void clearUpdated() { updated = 0; }
};
