#pragma once

#include <stdint.h>
#include <string.h>

#include "macros.h"

// ---------------------------------------------------------------------------
//   ParseValueResult
// ---------------------------------------------------------------------------

struct ParseValueResult {
public:
  enum State : uint8_t {
    NO_RESULT = 0,
    KEY_FOUND = 1 << 0,
    VALUE_CONVERTED = 1<< 1,
    VALUE_UPDATED = 1 << 2,
    // BEGINING OF PARSE_MASK
    STRING_PARSED = 8, // parsed JSON type, no error
    INTEGER_PARSED = 16, // parsed JSON type, no error
    FLOAT_PARSED = 24,// parsed JSON type, no error
    BOOLEAN_PARSED = 32,// parsed JSON type, no error
    NULL_VALUE_PARSED = 40,// parsed JSON type, no error
    ARRAY_PARSED = 48,// parsed JSON type, no error
    OBJECT_PARSED = 56, // parsed JSON type, no error
    // BEGINING OF ERROR_MASK
    PARSE_ERROR_STRING = 72, // error parsing JSON type String
    PARSE_ERROR_NUMERIC = 80, // error parsing JSON type Integer
    PARSE_ERROR_FLOAT = 88, // error parsing JSON type Float
    PARSE_ERROR_BOOLEAN = 96, // error parsing JSON type Boolean
    PARSE_ERROR_NULL = 104, // error parsing JSON type Null
    PARSE_ERROR_ARRAY = 112, // error parsing JSON type Array
    PARSE_ERROR_OBJECT = 120, // error parsing JSON type Object
    PARSE_ERROR_OBJECT_NO_START = 128,
    PARSE_ERROR_OBJECT_NO_END = 136,
    PARSE_ERROR_ARRAY_NO_START = 144, // custom error
    PARSE_ERROR_ARRAY_NO_END = 152, // custom error
    PARSE_ERROR_ARRAY_NO_COMMA = 160,
    PARSE_ERROR_OVERFLOW = 168,
    PARSE_ERROR_STRING_NO_START = 176,
    PARSE_ERROR_STRING_NO_END = 184,
    PARSE_ERROR_STRING_ESCAPE = 192,
    PARSE_ERROR_OBJECT_NO_COMMA = 200,
    PARSE_ERROR_UNKNOWN = 255 // unknown error
  };

  static constexpr uint8_t PARSE_MASK = 0x03;
  static constexpr uint8_t ERROR_MASK = 0xF8;

  // Constructeurs
//  constexpr ParseValueResult(uint16_t r) : _result(r) {}
  constexpr ParseValueResult() : _state(State::NO_RESULT) {}
  constexpr ParseValueResult(State s) : _state(s) {}
  constexpr ParseValueResult(uint8_t s) : _state(static_cast<State>(s)) {}

  constexpr bool keyFound() const {
    return (static_cast<uint8_t>(_state) & KEY_FOUND) != 0;
  }

  constexpr bool parsed() const {
    uint8_t error = static_cast<uint8_t>(_state) & ERROR_MASK;
    return error >= STRING_PARSED && error <= OBJECT_PARSED;
  }

  constexpr bool converted() const {
    return (static_cast<uint8_t>(_state) & VALUE_CONVERTED) != 0;
  }

  constexpr bool updated() const {
    return (static_cast<uint8_t>(_state) & VALUE_UPDATED) != 0;
  }

  constexpr State error() const {
    return static_cast<State>(_state & ERROR_MASK);
  }

  constexpr State state() const {
    return _state;
  }

  constexpr State get_state(State otherState) const {
    // In this situation, otherState is not a mask but one of the State enum values
    uint8_t state = static_cast<uint8_t>(_state);
    state |= (otherState & ~ERROR_MASK);

    if (otherState & ERROR_MASK) {
      state |= (state & ~ERROR_MASK) | (otherState & ERROR_MASK);
    }

    return static_cast<State>(state);
  }

  // Opérateur | (OR)

  constexpr ParseValueResult
  operator|(const ParseValueResult &other) const {
    State state = get_state(other._state);
    return ParseValueResult(state);
  }

  constexpr ParseValueResult
  operator|(const ParseValueResult::State &otherState) const {
    State state = get_state(otherState);
    return ParseValueResult(state);
  }

  // Opérateur |= (OR assignment)

  constexpr ParseValueResult &operator|=(const ParseValueResult &other) {
    _state = get_state(other._state);
    return *this;
  }

  constexpr ParseValueResult &operator|=(const ParseValueResult::State &otherState) {
    _state = get_state(otherState);
    return *this;
  }

  constexpr bool operator==(const ParseValueResult &other) const {
    return _state == other._state;
  }

  constexpr bool operator!=(const ParseValueResult &other) const {
    return _state != other._state;
  }

  void print() {

  }

private:
  State _state;
};

NAMESPACE_JSON_BEGIN

static const char *parseErrorToString(ParseValueResult::State &state) {

  uint8_t error = static_cast<uint8_t>(state) & ParseValueResult::ERROR_MASK;

  switch (error) {
  case ParseValueResult::NO_RESULT:
    return "NO_RESULT";
  case ParseValueResult::BOOLEAN_PARSED:
    return "BOOLEAN_PARSED";
  case ParseValueResult::INTEGER_PARSED:
    return "INTEGER_PARSED";
  case ParseValueResult::FLOAT_PARSED:
    return "FLOAT_PARSED";
  case ParseValueResult::STRING_PARSED:
    return "STRING_PARSED";
  case ParseValueResult::ARRAY_PARSED:
    return "ARRAY_PARSED";
  case ParseValueResult::OBJECT_PARSED:
    return "OBJECT_PARSED";
  case ParseValueResult::NULL_VALUE_PARSED:
    return "NULL_VALUE_PARSED";
  case ParseValueResult::PARSE_ERROR_STRING:
    return "PARSE_ERROR_STRING";
  case ParseValueResult::PARSE_ERROR_NUMERIC:
    return "PARSE_ERROR_NUMERIC";
  case ParseValueResult::PARSE_ERROR_FLOAT:
    return "PARSE_ERROR_FLOAT";
  case ParseValueResult::PARSE_ERROR_BOOLEAN:
    return "PARSE_ERROR_BOOLEAN";
  case ParseValueResult::PARSE_ERROR_NULL:
    return "PARSE_ERROR_NULL";
  case ParseValueResult::PARSE_ERROR_ARRAY:
    return "PARSE_ERROR_ARRAY";
  case ParseValueResult::PARSE_ERROR_OBJECT:
    return "PARSE_ERROR_OBJECT";
  case ParseValueResult::PARSE_ERROR_ARRAY_NO_START:
    return "PARSE_ERROR_ARRAY_NO_START";
  case ParseValueResult::PARSE_ERROR_ARRAY_NO_END:
    return "PARSE_ERROR_ARRAY_NO_END";
  case ParseValueResult::PARSE_ERROR_ARRAY_NO_COMMA:
    return "PARSE_ERROR_ARRAY_NO_COMMA";
  case ParseValueResult::PARSE_ERROR_OVERFLOW:
    return "PARSE_ERROR_OVERFLOW";
  case ParseValueResult::PARSE_ERROR_STRING_NO_START:
    return "PARSE_ERROR_STRING_NO_START";
  case ParseValueResult::PARSE_ERROR_STRING_NO_END:
    return "PARSE_ERROR_STRING_NO_END";
  case ParseValueResult::PARSE_ERROR_STRING_ESCAPE:
    return "PARSE_ERROR_STRING_ESCAPE";
  case ParseValueResult::PARSE_ERROR_OBJECT_NO_START:
    return "PARSE_ERROR_OBJECT_NO_START";
  case ParseValueResult::PARSE_ERROR_OBJECT_NO_END:
    return "PARSE_ERROR_OBJECT_NO_END";
  case ParseValueResult::PARSE_ERROR_OBJECT_NO_COMMA:
    return "PARSE_ERROR_OBJECT_NO_COMMA";
  default:
    return "UNKNOWN_ERROR";
  }
}

const char *parseStateToString(ParseValueResult::State &state) {
  uint8_t parse_state = static_cast<uint8_t>(state) & ParseValueResult::PARSE_MASK;

  if (parse_state == ParseValueResult::NO_RESULT) {
    return "NO_RESULT";
  } else if (parse_state & ParseValueResult::VALUE_UPDATED) {
    return "VALUE_UPDATED";
  } else if (parse_state & ParseValueResult::VALUE_CONVERTED) {
    return "VALUE_CONVERTED";
  }
  
  return "VALUE_NOT_CONVERTED_NOT_UPDATED";
}

[[maybe_unused]] static const char *errorToString(ParseValueResult &result) {
  static char output[80] = {0};

  ParseValueResult::State state = result.state();
  bool key_found = static_cast<uint8_t>(state) & ParseValueResult::KEY_FOUND;

  snprintf(output, sizeof(output), "%s %s %s", key_found ? "KEY_FOUND" : "KEY_NOT_FOUND", parseStateToString(state), parseErrorToString(state));

  return output;
}

NAMESPACE_JSON_END
