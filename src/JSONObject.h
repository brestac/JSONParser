#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "StreamCursor.h"
#include "StaticString.h"
#include "str_length.h"
#include "types.h"

using namespace JSON;
static size_t instances_counter = 0;

struct JSONObject {

public:
  uint32_t updated = 0;

  JSONObject() {
    ++instances_counter;
    JSON_DEBUG_COLOR(COLOR_BLUE, "New JSONObject subclass instance #%zu\n", instances_counter);
  }

  virtual ~JSONObject() {
    --instances_counter;
    if (instances_counter == 0) {
      //JSON_DEBUG_COLOR(COLOR_RED, "No more JSONObject, clear all StaticString<T>\n");
      clear_all();
    }
  }
  
  // Constructeur de copie : le membre copié est aussi une instance vivante
  JSONObject(const JSONObject& other) : updated(other.updated) {
    JSON_DEBUG_COLOR(COLOR_BLUE, "Copy JSONObject instance #%zu\n", instances_counter);
    ++instances_counter;
  }
  
  JSONObject& operator=(const JSONObject& other) {
    updated = other.updated;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  //  fromJSON
  ////////////////////////////////////////////////////////////////////////////////

  virtual JSON::ParseResult fromJSON(const char* /*name*/, const PointerCursorReader & /*cursor*/) {
    return ParseResult();
  }

  virtual JSON::ParseResult fromJSON(const char* /*name*/, StreamCursor & /*cursor*/) {
    return ParseResult();
  }

  template <size_t N>
  JSON::ParseResult fromJSON(const char* name, const char (&input)[N]) {
    JSON_DEBUG_WARNING("JSONObject::fromJSON(const char (&input)[N])\n");
    const PointerCursorReader cursor(input, N - 1);
    return fromJSON(name, cursor);
  }

  JSON::ParseResult fromJSON(const char* name, const char* input) {
    JSON_DEBUG_WARNING("JSONObject::fromJSON(const char* input)\n");
    const PointerCursorReader cursor(input, str_length(input, MAX_JSON_LENGTH));
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
  virtual size_t toJSON(PointerCursorWriter & cursor, bool /*updates*/ = false) {
    return cursor.write("{}");
  }

  virtual size_t toJSON(StreamCursor & cursor, bool /*updates*/ = false) {
    return cursor.write("{}");
  }

  template <typename T> std::enable_if_t<is_stream_v<T>, size_t> toJSON(T& output, bool updates = false) {
    StreamCursor cursor(output);
    return toJSON(cursor, updates);
  }

  void clearUpdated() { updated = 0; }
};