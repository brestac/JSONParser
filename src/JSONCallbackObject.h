#pragma once

#include <functional>

#include "JSONKey.h"
#include "JSONValue.h"
#include "ParseResult.h"
#include "StreamCursor.h"

using JSONCallback = std::function<void(const JSONKey &, const JSONValue &, bool &)>;

struct JSONCallbackObject {
  JSONCallback callback;
  JSONKey key;
  bool stop;

  JSON::ParseResult fromJSON(const char* name, JSON::StreamCursor &cursor);
  JSON::ParseResult fromJSON(const char* name, const JSON::PointerCursorReader &cursor);

  size_t toJSON(JSON::PointerCursorWriter &cursor, bool /*updates*/ = true) { return cursor.write("null"); }

  JSONCallbackObject(JSONCallback callback, JSONKey key) : callback(callback), key(key), stop(false) {
    JSON_DEBUG_INFO("JSONCallbackObject created\n");
  }

  void run(const JSONValue &value) {
    if (callback) {
      JSON_DEBUG_INFO("JSONCallbackObject running callback with key %.*s _array_index=%d\n", (int)key.length(),
                      key.data(), key.getArrayIndex());
      callback(key, value, stop);
      if (stop) {
        JSON_DEBUG_INFO("JSONCallbackObject stopped\n");
      }
    }
  }

  void setArrayIndex(int anIndex) {
    JSON_DEBUG_INFO("JSONCallbackObject setArrayIndex %d\n", anIndex);
    this->key.setArrayIndex(anIndex);
  }

  void setKey(const char* str, size_t len) {
    JSON_DEBUG_INFO("JSONCallbackObject setKey %.*s\n", (int)len, str);
    std::string_view key = JSON::StaticString<JSONCallbackObject>::string_view(str, len);
    this->key.setKey(key);
  }
};
