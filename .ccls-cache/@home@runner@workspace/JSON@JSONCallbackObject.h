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

  JSON::ParseResult fromJSON(JSON::PointerCursor cursor);

  JSONCallbackObject() : callback(nullptr), key(), stop(false), array_index(-1) {}
  JSONCallbackObject(JSONCallback callback, JSONKey key) : callback(callback), key(key), stop(false), array_index(-1) {
    JSON_DEBUG_INFO("JSONCallbackObject created\n");
  }

  void run(JSONValue value, int arrayIndex) {
    if (callback) {
      key.setArrayIndex(arrayIndex);
      callback(key, value, stop);
      if (stop) {
        JSON_DEBUG_INFO("JSONCallbackObject stopped\n");
      }
    }
  }
};
