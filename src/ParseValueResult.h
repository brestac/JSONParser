#pragma once
// ---------------------------------------------------------------------------
//   ParseValueResult
// ---------------------------------------------------------------------------
struct ParseValueResult {
  enum Flag : uint8_t {
    NO_RESULT = 0,
    KEY_FOUND = 1 << 0,
    VALUE_PARSED = 1 << 1,
    VALUE_CONVERTED = 1 << 2,
    VALUE_UPDATED = 1 << 3
  };

  uint8_t value;

  // Constructeurs
  constexpr ParseValueResult() : value(0) {}
  constexpr ParseValueResult(Flag f) : value(f) {}
  constexpr ParseValueResult(uint8_t v) : value(v) {}

  // Opérateur | (OR)
  constexpr ParseValueResult operator|(const ParseValueResult &other) const {
    return ParseValueResult(value | other.value);
  }

  constexpr ParseValueResult operator|(Flag f) const {
    return ParseValueResult(value | f);
  }

  // Opérateur |= (OR assignment)
  constexpr ParseValueResult &operator|=(const ParseValueResult &other) {
    value |= other.value;
    return *this;
  }

  constexpr ParseValueResult &operator|=(Flag f) {
    value |= f;
    return *this;
  }

  // Opérateur & (AND)
  constexpr ParseValueResult operator&(const ParseValueResult &other) const {
    return ParseValueResult(value & other.value);
  }

  constexpr ParseValueResult operator&(Flag f) const {
    return ParseValueResult(value & f);
  }

  // Opérateur &= (AND assignment)
  constexpr ParseValueResult &operator&=(const ParseValueResult &other) {
    value &= other.value;
    return *this;
  }

  constexpr ParseValueResult &operator&=(Flag f) {
    value &= f;
    return *this;
  }

  // Opérateur ~ (NOT)
  constexpr ParseValueResult operator~() const {
    return ParseValueResult(~value);
  }

  constexpr explicit operator bool() const { return value != 0; }

  constexpr operator uint8_t() const { return value; }

  constexpr bool key() const { return (value & KEY_FOUND) != 0; }
  constexpr bool parsed() const { return (value & VALUE_PARSED) != 0; }
  constexpr bool converted() const { return (value & VALUE_CONVERTED) != 0; }
  constexpr bool updated() const { return (value & VALUE_UPDATED) != 0; }

  void print() {
    JSON_DEBUG_PRINTF(
        "ParseValueResult: KEY_FOUND=%d VALUE_PARSED=%d VALUE_CONVERTED=%d VALUE_UPDATED=%d\n",
        (value & KEY_FOUND) != 0, (value & VALUE_PARSED) != 0,
        (value & VALUE_CONVERTED) != 0, (value & VALUE_UPDATED) != 0);
  }
};
