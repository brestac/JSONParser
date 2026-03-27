#pragma once
// ---------------------------------------------------------------------------
//   CallBack Object
// ---------------------------------------------------------------------------
#include "JSONKey.h"
#include "JSONValue.h"

using JSONCallback = std::function<void(JSONKey, JSONValue, bool &)>;

struct JSONCallbackObject {
  JSONCallback callback;
  JSONKey key;
  bool stop;
  int array_index;

#ifdef ARDUINO
  JSON::ParseResult fromJSON(JSON::StreamCursor& cursor);
#else
  JSON::ParseResult fromJSON(const JSON:: PointerCursorReader& cursor);
  size_t toJSON(JSON::PointerCursorWriter& cursor, bool updates = true) {
    return cursor.write("null");
  }
#endif
  JSONCallbackObject() : callback(nullptr), key(), stop(false), array_index(-1) {}
  JSONCallbackObject(JSONCallback callback, JSONKey key) : callback(callback), key(key), stop(false), array_index(-1) {
    JSON_DEBUG_INFO("JSONCallbackObject created\n");
  }

  void run(JSONValue value, int arrayIndex) {
    if (callback) {
      JSON_DEBUG_INFO("JSONCallbackObject running callback with key %.*s array_index=%d\n", (int)key.length(), key.data(), arrayIndex);
      key.setArrayIndex(arrayIndex);
      callback(key, value, stop);
      if (stop) {
        JSON_DEBUG_INFO("JSONCallbackObject stopped\n");
      }
    }
  }

  void setArrayIndex(int index) {
    array_index = index;
    JSON_DEBUG_INFO("JSONCallbackObject setArrayIndex %d\n", index);
  }
};
