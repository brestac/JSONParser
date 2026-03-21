#pragma once

// ---------------------------------------------------------------------------
//   UnknownValueType
// ---------------------------------------------------------------------------
#include <string.h>
#include "JSONData.h"

struct ParseResult;
struct UnknownValueType : JSONData {

  constexpr UnknownValueType() = default;
  constexpr bool operator==(const UnknownValueType &other) const { return true; }
  constexpr bool operator!=(const UnknownValueType &other) const { return false; }
  JSON::ParseResult fromJSON(JSON::PointerCursor cursor);
  
  size_t toJSON(JSON::PointerCursorWriter writer, bool updates) {
    return writer.write("null", 4UL);
  }

  using JSONData::fromJSON;
  using JSONData::toJSON;
};
