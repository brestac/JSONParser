#pragma once

// ---------------------------------------------------------------------------
//   UnknownValueType
// ---------------------------------------------------------------------------
#include "JSONObject.h"

struct ParseResult;

struct UnknownValueType : JSONObject {
  using JSONObject::fromJSON;
  // using JSONObject::toJSON;

  constexpr UnknownValueType() = default;
  JSON::ParseResult fromJSON(const char* name, JSON::StreamCursor &cursor);
  JSON::ParseResult fromJSON(const char* name, const JSON::PointerCursorReader &cursor);

  // ─── toJSON
  // ───────────────────────────────────────────────────────────────────────
  size_t toJSON(JSON::StreamCursor &writer, bool updates) { return writer.write("null"); }

  size_t toJSON(JSON::PointerCursorWriter &writer, bool updates) { return writer.write("null"); }

  constexpr bool operator==(const UnknownValueType &other) const { return true; }
  constexpr bool operator!=(const UnknownValueType &other) const { return false; }
};
