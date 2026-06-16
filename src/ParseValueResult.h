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
    PARSE_ERROR = 0, // Not parsed, not converted, not updated
    VALUE_PARSED = 1,  // Parsed, not converted, not updated
    VALUE_CONVERTED = 2, // Parsed, converted, not updated
    VALUE_UPDATED = 3, // Parsed, converted, updated
    KEY_FOUND = 4, // Key found mask
    STRING = 8, // detected JSON type, no error
    INTEGER = 16, // detected JSON type, no error
    FLOAT = 24,// detected JSON type, no error
    BOOLEAN = 32,// detected JSON type, no error
    NULL_VALUE = 40,// detected JSON type, no error
    ARRAY = 48,// detected JSON type, no error
    OBJECT = 56, // detected JSON type, no error
    ERROR_1 = 64, // Type based parse error, exclusive with other errors AND with detected JSON type (4 to 56)
    ERROR_2 = 72,
    ERROR_3 = 80,
    ERROR_4 = 88,
    ERROR_5 = 96,
    ERROR_6 = 104,
    ERROR_7 = 112,
    ERROR_8 = 120,
    ERROR_9 = 128,
    ERROR_10 = 136
  };

  static constexpr uint8_t PARSE_MASK = 0x03;
  static constexpr uint8_t ERROR_OFFSET = 64;
  static constexpr uint8_t ERROR_MASK = 0xF8;

  bool parsed() const {
    // Get bit 0 and 1 of _state. Result should be 0 or 1 or 2 or 3
    uint8_t parse_state = _state & PARSE_MASK;
    return parse_state == VALUE_PARSED || parse_state == VALUE_CONVERTED || parse_state == VALUE_UPDATED;
  }

  bool converted() const {
    // Get bit 0 and 1 of _state. Result should be 0 or 1 or 2 or 3
    uint8_t parse_state = _state & PARSE_MASK;
    return parse_state == VALUE_CONVERTED || parse_state == VALUE_UPDATED;
  }

  bool updated() const {
    // Get bit 0 and 1 of _state. Result should be 0 or 1 or 2 or 3
    uint8_t parse_state = _state & PARSE_MASK;
    return parse_state == VALUE_UPDATED;
  }

  bool keyFound() const {
    return (_state & KEY_FOUND) != 0;
  }

  uint8_t error() const {
    return (_state & ERROR_MASK) >> 3;
  }

  ParseValueResult set_error(uint8_t error) {
    _state = (_state & ~ERROR_MASK) | (error << 3);
    return *this;
  }

  ParseValueResult set_parsed() {
    if (!parsed()) {
      _state |= VALUE_PARSED;
    }
    return *this;
  }

  ParseValueResult set_converted() {
    if (!converted()) {
      _state |= VALUE_CONVERTED;
    }
    return *this;
  }

  ParseValueResult set_updated() {
    if (!updated()) {
      _state |= VALUE_UPDATED;
    }

    return *this;
  }

  // Constructeurs
//  constexpr ParseValueResult(uint16_t r) : _result(r) {}
  constexpr ParseValueResult(uint8_t s) : _state(s) {}

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
    
    if (otherState == VALUE_PARSED) {
      if ((state & PARSE_MASK) == PARSE_ERROR) {
        state |= VALUE_PARSED;
      }
    } else if (otherState == VALUE_CONVERTED) {
      if ((state & PARSE_MASK) == VALUE_PARSED) {
        state |= VALUE_CONVERTED;
      }
    } else if (otherState == VALUE_UPDATED) {
      if ((state & PARSE_MASK) == VALUE_CONVERTED) {
        state |= VALUE_UPDATED;
      }
    } else if (otherState == KEY_FOUND) {
      state |= KEY_FOUND;
    } else if (otherState >= STRING) {
      state = (state & ~ERROR_MASK) | otherState;
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

  constexpr operator uint8_t() const { return _state; }

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

static const char *parseErrorToString(ParseValueResult::State &state) {

  uint8_t error = state & ParseValueResult::ERROR_MASK;
  
  switch (error) {
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
  default:
    return "UNKNOWN_ERROR";
  }
}

const char *parseStateToString(ParseValueResult::State &state) {
  uint8_t parse_state = state & ParseValueResult::PARSE_MASK;
  switch (parse_state) {
    case ParseValueResult::PARSE_ERROR:
      return "PARSE_ERROR";
    case ParseValueResult::VALUE_PARSED:
      return "VALUE_PARSED";
    case ParseValueResult::VALUE_CONVERTED:
      return "VALUE_CONVERTED";
    case ParseValueResult::VALUE_UPDATED:
      return "VALUE_UPDATED";
    default:
      return "UNKNOWN_PARSE_STATE";
  }
}

static const char *errorToString(ParseValueResult::State &state) {
  static char output[32] = {0};

  bool key_found = state & ParseValueResult::KEY_FOUND;
  strcpy(output, key_found ? "KEY_FOUND | " : "KEY_NOT_FOUND | ");
  strcat(output, parseStateToString(state));
  strcat(output, " | ");
  strcat(output, parseErrorToString(state));

  return output;
}

NAMESPACE_JSON_END
