#pragma once

#include <stdint.h>
#include <string.h>

#include "macros.h"

// ---------------------------------------------------------------------------
//   ParseValueResult
// ---------------------------------------------------------------------------

struct ParseValueResult {
public:
  // enum Result : uint16_t {
  //   NO_RESULT = 0,
  //   KEY_FOUND = 1 << 0,
  //   VALUE_PARSED = 1 << 1,
  //   VALUE_CONVERTED = 1 << 2,
  //   VALUE_UPDATED = 1 << 3,
  //   UNKNOWN = 1 << 4,
  //   STRING = 1 << 5,
  //   INTEGER = 1 << 6,
  //   FLOAT = 1 << 7,
  //   BOOLEAN = 1 << 8,
  //   NULL_VALUE = 1 << 9,
  //   POINTER = 1 << 10,
  //   ARRAY = 1 << 11,
  //   OBJECT = 1 << 12
  // };

  enum State : uint8_t {
    NO_RESULT = 0,
    KEY_FOUND = 1 << 0,
    VALUE_CONVERTED = 1<< 1,
    VALUE_UPDATED = 1 << 2,
    // BEGINING OF ERRORS 8 to 56 means no error
    STRING_PARSED = 1 << 3, // parsed JSON type, no error
    NUMERIC_PARSED = 16, // parsed JSON type, no error
    FLOAT_PARSED = 24,// parsed JSON type, no error
    BOOLEAN_PARSED = 32,// parsed JSON type, no error
    NULL_VALUE_PARSED = 40,// parsed JSON type, no error
    ARRAY_PARSED = 48,// parsed JSON type, no error
    OBJECT_PARSED = 56, // parsed JSON type, no error
    PARSE_ERROR_STRING = 72, // error parsing JSON type String
    PARSE_ERROR_NUMERIC = 80, // error parsing JSON type Integer
    PARSE_ERROR_FLOAT = 88, // error parsing JSON type Float
    PARSE_ERROR_BOOLEAN = 96, // error parsing JSON type Boolean
    PARSE_ERROR_NULL = 104, // error parsing JSON type Null
    PARSE_ERROR_ARRAY = 112, // error parsing JSON type Array
    PARSE_ERROR_OBJECT = 120, // error parsing JSON type Object
    CUSTOM_ERROR_1 = 128, // custom error
    CUSTOM_ERROR_2 = 136, // custom error
    CUSTOM_ERROR_3 = 144, // custom error
    CUSTOM_ERROR_4 = 152, // custom error
    CUSTOM_ERROR_5 = 160, // custom error
    CUSTOM_ERROR_6 = 168, // custom error
    CUSTOM_ERROR_7 = 176, // custom error
    CUSTOM_ERROR_8 = 184,
    CUSTOM_ERROR_9 = 192,
    CUSTOM_ERROR_10 = 200,
    CUSTOM_ERROR_11 = 208,
    CUSTOM_ERROR_12 = 216,
    CUSTOM_ERROR_13 = 224,
    CUSTOM_ERROR_14 = 232,
    CUSTOM_ERROR_15 = 240,
    CUSTOM_ERROR_16 = 248,
    PARSE_ERROR_UNKNOWN = 255 // unknown error
  };

  static constexpr uint8_t PARSE_MASK = 0x03;
  static constexpr uint8_t ERROR_OFFSET = 3;
  static constexpr uint8_t ERROR_MASK = 0xF8;

  // Constructeurs
//  constexpr ParseValueResult(uint16_t r) : _result(r) {}
  constexpr ParseValueResult() : _state(0) {}
  constexpr ParseValueResult(uint8_t s) : _state(s) {}

  bool keyFound() const {
    return (_state & KEY_FOUND) != 0;
  }
  
  bool parsed() const {
    uint8_t error = _state & ERROR_MASK;
    return error >= STRING_PARSED && error <= OBJECT_PARSED;
  }

  bool converted() const {
    return (_state & VALUE_CONVERTED) != 0;
  }

  bool updated() const {
    return (_state & VALUE_UPDATED) != 0;
  }

  uint8_t error() const {
    return _state & ERROR_MASK;
  }

  uint8_t state() const {
    return _state;
  }

  // // Opérateur | (OR)
  // constexpr ParseValueResult operator|(const ParseValueResult &other) const {
  //   //return ParseValueResult(_result | other._result);
  //   // Replace the last 3 bits of _state with the last 3 bits of other._state
  //   // Set the first 5 bits of _state to the first 5 bits of other._state
  //   uint8_t parse_state = other._state & ((1 << 0) | (1 << 1));
  //   uint8_t key_found = other._state & KEY_FOUND;
  //   uint8_t error = other._state & ERROR_MASK;
  //   return ParseValueResult((_state & ~((1 << 0) | (1 << 1) | KEY_FOUND | ERROR_MASK)) | parse_state | key_found | error);
  // }
  uint8_t get_state(uint8_t otherState) const {
    // In this situation, otherState is not a mask but one of the State enum values
    uint8_t state = _state;
    state |= (otherState & ~ERROR_MASK);
    
    if (otherState & ERROR_MASK) {
      state |= (state & ~ERROR_MASK) | (otherState & ERROR_MASK);
    }
    
    return state;
  }

  constexpr ParseValueResult
  operator|(const ParseValueResult::State &otherState) const {
    uint8_t state = get_state(otherState);
    return ParseValueResult(state);
  }
  // Opérateur |= (OR assignment)

  constexpr ParseValueResult &operator|=(const ParseValueResult &other) {
    _state = get_state(other._state);
    return *this;
  }

  // constexpr ParseValueResult &
  // operator|=(const ParseValueResult::Result &otherResult) {
  //   _result |= otherResult;
  //   return *this;
  // }

  // // Opérateur & (AND)
  // constexpr ParseValueResult operator&(const ParseValueResult &other) const {
  //   return ParseValueResult(_result | other._result);
  // }

  // constexpr ParseValueResult
  // operator&(const ParseValueResult::Result &otherResult) const {
  //   return ParseValueResult(_result & otherResult);
  // }

  // constexpr operator uint8_t() const { return _state; }

  // constexpr ParseValueResult::Result result() const {
  //   return (ParseValueResult::Result)_result;
  // }
  // constexpr bool keyFound() const { return (_result & KEY_FOUND) != 0; }
  // constexpr bool parsed() const { return (_result & VALUE_PARSED) != 0; }
  // constexpr bool converted() const { return (_result & VALUE_CONVERTED) != 0; }
  // constexpr bool updated() const { return (_result & VALUE_UPDATED) != 0; }

  // constexpr uint16_t valueType() const {
  //   return (_result &~ (KEY_FOUND | VALUE_PARSED | VALUE_CONVERTED | VALUE_UPDATED));
  // }

  void print() {

  }

private:
  //uint16_t _result;
  uint8_t _state;
};

NAMESPACE_JSON_BEGIN

static const char *parseErrorToString(uint8_t &state) {

  uint8_t error = state & ParseValueResult::ERROR_MASK;
  
  switch (error) {
  case ParseValueResult::BOOLEAN_PARSED:
    return "BOOLEAN";
  case ParseValueResult::NUMERIC_PARSED:
    return "NUMERIC";
  case ParseValueResult::FLOAT_PARSED:
    return "FLOAT";
  case ParseValueResult::STRING_PARSED:
    return "STRING";
  case ParseValueResult::ARRAY_PARSED:
    return "ARRAY";
  case ParseValueResult::OBJECT_PARSED:
    return "OBJECT";
  case ParseValueResult::NULL_VALUE_PARSED:
    return "NULL";
  default:
    return "UNKNOWN_ERROR";
  }
}

const char *parseStateToString(uint8_t &state) {
  uint8_t parse_state = state & ParseValueResult::PARSE_MASK;
  switch (parse_state) {
    case ParseValueResult::NO_RESULT:
      return "NO_RESULT";
    case ParseValueResult::VALUE_CONVERTED:
      return "VALUE_CONVERTED";
    case ParseValueResult::VALUE_UPDATED:
      return "VALUE_UPDATED";
    default:
      return "UNKNOWN_PARSE_STATE";
  }
}

static const char *errorToString(ParseValueResult &result) {
  static char output[32] = {0};

  uint8_t state = result.state();
  bool key_found = state & ParseValueResult::KEY_FOUND;
  strcpy(output, key_found ? "KEY_FOUND | " : "KEY_NOT_FOUND | ");
  strcat(output, parseStateToString(state));
  strcat(output, " | ");
  strcat(output, parseErrorToString(state));

  return output;
}

NAMESPACE_JSON_END
