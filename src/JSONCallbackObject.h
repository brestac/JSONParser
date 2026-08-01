#pragma once

#include <functional>

#include "JSONKey.h"
#include "JSONValue.h"
#include "ParseResult.h"
#include "StreamCursor.h"

NAMESPACE_JSON_BEGIN

enum SKIP : uint8_t {
    NONE = 0,
    END = 1,
    STOP = 2
};

NAMESPACE_JSON_END

using JSONCallback =
    std::function<void(const JSONKey &, const JSONValue &, JSON::SKIP &)>;

struct JSONCallbackObject {
  JSONCallback callback;
  JSONKey key;
  JSON::SKIP skip = JSON::SKIP::NONE;

  JSON::ParseResult fromJSON(JSON::StreamCursorReader &cursor);
  JSON::ParseResult fromJSON(const JSON::PointerCursorReader &cursor);
  template <typename Parser> JSON::ParseResult _fromJSON(Parser& parser);

  size_t toJSON(JSON::PointerCursorWriter &cursor, bool /*updates*/ = true) {
    return cursor.write("null");
  }

  JSONCallbackObject(JSONCallback callback, JSONKey& key)
      : callback(callback), key(key), skip(JSON::SKIP::NONE) {
    JSON_DEBUG_INFO("JSONCallbackObject created\n");
  }

  template <size_t N>
  JSONCallbackObject(JSONCallback callback, const char (&keyStr)[N])
      : callback(callback), key(keyStr), skip(JSON::SKIP::NONE) {
    JSON_DEBUG_INFO("JSONCallbackObject created from string\n");
  }

  void run(const JSONValue &value) {
    if (callback) {
      JSON_DEBUG_INFO("JSONCallbackObject running callback with key %.*s _array_index=%d\n", (int)key.length(), key.data(), key.getArrayIndex());
      callback(key, value, skip);
      if (skip == JSON::SKIP::STOP) {
        JSON_DEBUG_INFO("JSONCallbackObject stopped\n");
      }
    }
  }

  void setArrayIndex(int16_t anIndex) {
    JSON_DEBUG_INFO("JSONCallbackObject setArrayIndex %d\n", anIndex);
    this->key.setArrayIndex(anIndex);
  }

  void setKey(char* str, size_t len) {
    JSON_DEBUG_INFO("JSONCallbackObject setKey %.*s\n", (int)len, str);
     std::string_view key(str, len);
     this->key.setKey(key);
  }

  void push() {
    this->key._array_index.push(this->key.getArrayIndex());
  }

  void pop() {
    this->key._array_index.pop();
  }
};
