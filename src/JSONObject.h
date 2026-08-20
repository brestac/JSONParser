#pragma once

#include "ParseResult.h"
#include "PointerCursor.h"
#include "StreamCursor.h"
#include "StringPool.h"
#include "str_length.h"
#include "types.h"

using namespace JSON;

NAMESPACE_JSON_BEGIN
template <typename TargetT, typename Cursor>
static TargetT fromJSON(Cursor& cursor)                        
     {                                                               
  TargetT obj;
  obj.fromJSON(cursor);                             
  return obj;
}
NAMESPACE_JSON_END

static size_t instances_counter = 0;

struct JSONObject {
public:
  uint8_t updated = 0;

  JSONObject() {
    ++instances_counter;
    JSON_DEBUG_COLOR(COLOR_BLUE, "New JSONObject subclass instance #%zu\n", instances_counter);
  }

  virtual ~JSONObject() {
    --instances_counter;
    if (instances_counter == 0) {
      //JSON_DEBUG_COLOR(COLOR_RED, "No more JSONObject, clear all StringPool<T>\n");
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

  ////////////////////////////////////////////////////////////////////////////////
  //  toJSON
  ////////////////////////////////////////////////////////////////////////////////
  virtual size_t toJSON(PointerCursorWriter & cursor, bool /*updates*/ = false) {
    return cursor.write("{}");
  }

  virtual size_t toJSON(StreamCursorWriter & cursor, bool /*updates*/ = false) {
    return cursor.write("{}");
  }

  template <typename T> std::enable_if_t<is_stream_v<T>, size_t> toJSON(T& output, bool updates = false) {
    StreamCursorWriter cursor(output);
    return toJSON(cursor, updates);
  }

  void clearUpdated() { updated = 0; }
};