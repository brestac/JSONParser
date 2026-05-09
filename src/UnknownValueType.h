#pragma once

// ---------------------------------------------------------------------------
//   UnknownValueType
// ---------------------------------------------------------------------------
#include "JSONObject.h"

struct ParseResult;
struct UnknownValueType : JSONObject {
  using JSONObject::fromJSON;
  using JSONObject::toJSON;

  constexpr UnknownValueType() = default;

  JSON::ParseResult fromJSON(const JSON:: PointerCursorReader& cursor) override;

  size_t toJSON(JSON::PointerCursorWriter& writer, bool updates) override {
    return writer.write("null");
  }

#ifdef ARDUINO
  JSON::ParseResult fromJSON(JSON::StreamCursor& cursor) override;
  size_t toJSON(JSON::StreamCursor& writer, bool updates) override {
    return writer.write("null");
  }
#else
  size_t toJSON(JSON::PointerCursorPrinter& writer, bool updates) override {
    return writer.write("null");
  }
#endif

  constexpr bool operator==(const UnknownValueType &other) const { return true; }
  constexpr bool operator!=(const UnknownValueType &other) const { return false; }
};
