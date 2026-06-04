#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "StreamCursor.h"
#include "str_length.h"
#include "types.h"

using namespace JSON;

struct JSONObject {
public:
  uint32_t updated = 0;

  virtual ~JSONObject() = default;

  ////////////////////////////////////////////////////////////////////////////////
  //  fromJSON
  ////////////////////////////////////////////////////////////////////////////////

  virtual JSON::ParseResult fromJSON(const char* name, const PointerCursorReader &cursor) {
    return ParseResult();
  }

  virtual JSON::ParseResult fromJSON(const char* name, StreamCursor &cursor) {
    return ParseResult();
  }

  template <size_t N>
  ParseResult fromJSON(const char* name, const char (&input)[N]) {
    JSON_DEBUG_WARNING("JSONObject::fromJSON(const char (&input)[N])\n");
    const PointerCursorReader cursor(input, N - 1);
    return fromJSON(name, cursor);
  }

  JSON::ParseResult fromJSON(const char* name, const char* input) {
    JSON_DEBUG_WARNING("JSONObject::fromJSON(const char* input)\n");
    const PointerCursorReader cursor(input, str_length(input, MAX_POINTER_CURSOR_SIZE));
    return fromJSON(name, cursor);
  }

  template <typename T>
  std::enable_if_t<is_stream_v<T>, ParseResult>
  fromJSON(const char* name, T& input) {
    StreamCursor cursor(input);
    return fromJSON(name, cursor);
  }

  template <typename T>
  ParseResult fromJSON(T& input) {
    return fromJSON("$ROOT", input);
  }

  template <typename T>
  std::enable_if_t<is_stream_v<T*>, ParseResult>
  fromJSON(T* input) {
    return fromJSON("$ROOT", *input);
  }

  ////////////////////////////////////////////////////////////////////////////////
  //  toJSON
  ////////////////////////////////////////////////////////////////////////////////
  virtual size_t toJSON(PointerCursorWriter &cursor, bool updates = true) {
    return 0;
  }

  virtual size_t toJSON(StreamCursor &cursor, bool updates = true) { return 0; }

  template <typename T>
  std::enable_if_t<is_stream_v<T>, size_t>
  toJSON(T& output, bool updates = true) {
    StreamCursor cursor(output);
    return toJSON(cursor, updates);
  }

  void clearUpdated() { updated = 0; }
};
