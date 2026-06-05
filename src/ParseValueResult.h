#pragma once
#include "macros.h"
#include <stdint.h>
// ---------------------------------------------------------------------------
//   ParseValueResult
// ---------------------------------------------------------------------------

struct ParseValueResult {
public:
  enum Result : uint16_t {
    NO_RESULT = 0,
    KEY_FOUND = 1 << 0,
    VALUE_PARSED = 1 << 1,
    VALUE_CONVERTED = 1 << 2,
    VALUE_UPDATED = 1 << 3,
    UNKNOWN = 1 << 4,
    STRING = 1 << 5,
    INTEGER = 1 << 6,
    FLOAT = 1 << 7,
    BOOLEAN = 1 << 8,
    NULL_VALUE = 1 << 9,
    POINTER = 1 << 10,
    ARRAY = 1 << 11,
    OBJECT = 1 << 12
  };

  // Constructeurs
  constexpr ParseValueResult() : _result(0) {}
  constexpr ParseValueResult(uint16_t r) : _result(r) {}

  // Opérateur | (OR)
  constexpr ParseValueResult operator|(const ParseValueResult &other) const {
    return ParseValueResult(_result | other._result);
  }

  constexpr ParseValueResult operator|(const ParseValueResult::Result &otherResult) const {
    return ParseValueResult(_result | otherResult);
  }
  // Opérateur |= (OR assignment)

  constexpr ParseValueResult &operator|=(const ParseValueResult &other) {
    _result |= other._result;
    return *this;
  }

  constexpr ParseValueResult &operator|=(const ParseValueResult::Result &otherResult) {
    _result |= otherResult;
    return *this;
  }

  // Opérateur & (AND)
  constexpr ParseValueResult operator&(const ParseValueResult &other) const {
    return ParseValueResult(_result | other._result);
  }

  constexpr ParseValueResult operator&(const ParseValueResult::Result &otherResult) const {
    return ParseValueResult(_result & otherResult);
  }

  constexpr operator uint16_t() const { return _result; }

  constexpr bool keyFound() const { return (_result & KEY_FOUND) != 0; }
  constexpr bool parsed() const { return (_result & VALUE_PARSED) != 0; }
  constexpr bool converted() const { return (_result & VALUE_CONVERTED) != 0; }
  constexpr bool updated() const { return (_result & VALUE_UPDATED) != 0; }

  constexpr uint16_t valueType() const {
    return (_result & (~(KEY_FOUND | VALUE_PARSED | VALUE_CONVERTED | VALUE_UPDATED)));
  }

  void print() {
    DEBUG_PRINTF("ParseValueResult: KEY_FOUND=%d VALUE_PARSED=%d "
                 "VALUE_CONVERTED=%d VALUE_UPDATED=%d _type=%hhu\n",
                 (_result & KEY_FOUND) != 0, (_result & VALUE_PARSED) != 0, (_result & VALUE_CONVERTED) != 0,
                 (_result & VALUE_UPDATED) != 0, valueType());
  }

private:
  uint8_t _result;
};

NAMESPACE_JSON_BEGIN

static const char* valueTypeToString(ParseValueResult result) {
  uint16_t type = result.valueType();

  switch (type) {
  case ParseValueResult::UNKNOWN:
    return "UNKNOWN";
  case ParseValueResult::BOOLEAN:
    return "BOOLEAN";
  case ParseValueResult::INTEGER:
    return "INTEGER";
  case ParseValueResult::FLOAT:
    return "FLOAT";
  case ParseValueResult::STRING:
    return "STRING";
  case ParseValueResult::ARRAY:
    return "ARRAY";
  case ParseValueResult::OBJECT:
    return "OBJECT";
  case ParseValueResult::POINTER:
    return "POINTER";
  default:
    return "UNKNOWN";
  }
}

NAMESPACE_JSON_END
