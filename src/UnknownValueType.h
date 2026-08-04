#pragma once

// ---------------------------------------------------------------------------
//   UnknownValueType
// ---------------------------------------------------------------------------
//#include "JSONObject.h"
#include "ParseResult.h"
#include "PointerCursor.h"
#include "StreamCursor.h"

struct ParseResult;

struct UnknownValueType {

  UnknownValueType() = default;

  template <typename T>
  JSON::ParseResult fromJSON(T& input);

  // ─── toJSON
  // ───────────────────────────────────────────────────────────────────────
  size_t toJSON(JSON::StreamCursorWriter &writer, bool /*updates*/) { return writer.write("null"); }
  size_t toJSON(JSON::PointerCursorWriter &writer, bool /*updates*/) { return writer.write("null"); }

  constexpr bool operator==(const UnknownValueType & /*other*/) const { return true; }
  constexpr bool operator!=(const UnknownValueType & /*other*/) const { return false; }
};

static UnknownValueType UNKNOW_VALUE_STATIC;
