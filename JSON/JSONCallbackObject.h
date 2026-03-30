#pragma once
// ---------------------------------------------------------------------------
//   CallBack Object
// ---------------------------------------------------------------------------
struct JSONKey;
struct JSONValue;
h"
#include "JSONKey.h"
#include "JSONValue.h"

using JSONCallback = std::function<void(JSONKey, JSONValue, bool &)>;

struct JSONCallbackObject {
  JSONCallback callback;
  JSONKey key;
  bool stop;

#ifdef ARDUINO
  JSON::ParseResult fromJSON(JSON::StreamCursor& cursor);
#else
  JSON::ParseResult fromJSON(const JSON:: PointerCursorReader& cursor);
  size_t toJSON(JSON::PointerCursorWriter& cursor, bool updates = true) {
    return cursor.write("null");
  }
#endif
  JSONCallbackObject(JSONCallback callback, JSONKey key) : callback(callback), key(key), stop(false) {
    JSON_DEBUG_INFO("JSONCallbackObject created\n");
  }

  void run(JSONValue value) {
    if (callback) {
      JSON_DEBUG_INFO("JSONCallbackObject running callback with key %.*s _array_index=%d\n", (int)key.length(), key.data(), key.getArrayIndex());
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

  void setKey(JSONKey aKey) {
    JSON_DEBUG_INFO("JSONCallbackObject setKey %.*s\n", (int)aKey.length(), aKey.data());
    this->key.setKey(aKey);
  }

  void setKey(const char *key, size_t len) {
    JSON_DEBUG_INFO("JSONCallbackObject setKey %.*s\n", (int)len, key);
    this->key.setKey(key, len);
  }
};
