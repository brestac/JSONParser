#pragma once

// ---------------------------------------------------------------------------
//   UnknownValueType
// ---------------------------------------------------------------------------
#include "JSONObject.h"

struct ParseResult;

struct UnknownValueType : JSONObject {
  using JSONObject::fromJSON;
  // using JSONObject::toJSON;

  UnknownValueType() = default;
  JSON::ParseResult fromJSON(JSON::StreamCursorReader &cursor);
  JSON::ParseResult fromJSON(const JSON::PointerCursorReader &cursor);
  template <typename Parser>
  JSON::ParseResult _fromJSON(Parser& parser);

  // ─── toJSON
  // ───────────────────────────────────────────────────────────────────────
  size_t toJSON(JSON::StreamCursorWriter &writer, bool /*updates*/) { return writer.write("null"); }

  size_t toJSON(JSON::PointerCursorWriter &writer, bool /*updates*/) { return writer.write("null"); }

  constexpr bool operator==(const UnknownValueType & /*other*/) const { return true; }
  constexpr bool operator!=(const UnknownValueType & /*other*/) const { return false; }
};

static UnknownValueType UNKNOW_VALUE_STATIC;
