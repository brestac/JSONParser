#pragma once

#include <string_view>

#include "utils.h"

// ---------------------------------------------------------------------------
//   hash32 — FNV-1a 32 bits, constexpr
// ---------------------------------------------------------------------------

constexpr uint32_t operator""_hash(const char* str, size_t len) {
  return hash32(str, len);
}

// ---------------------------------------------------------------------------
//   parse_int_constexpr — remplace strtol (non constexpr)
// ---------------------------------------------------------------------------

constexpr int16_t parse_int_constexpr(const char* str) {
  int16_t result = 0;
  size_t n = 0;
  while (*str >= '0' && *str <= '9' && ++n < JSON::MAX_KEY_LENGTH) {
    result = static_cast<int16_t>(result * 10 + (*str - '0'));
    ++str;
  }
  return result;
}

// ---------------------------------------------------------------------------
//   get_json_key_and_index — constexpr, sans strtol
//   Retourne {string_view sur la clé, index ou -1}
// ---------------------------------------------------------------------------

template <size_t N>
constexpr std::pair<std::string_view, int16_t>
get_json_key_and_index(const char (&raw_key)[N]) {
  for (size_t i = 0; i < N; ++i) {
    if (raw_key[i] == '[') {
      return {std::string_view(raw_key, i),
              parse_int_constexpr(raw_key + i + 1)};
    }
    if (raw_key[i] == '\0')
      break;
  }
  return {std::string_view(raw_key, N - 1), -1};
}

// Helpers pour extraire clé et index séparément (utilisés dans la
// liste d'initialisation du constructeur JSONKey)
template <size_t N>
constexpr std::string_view extract_key(const char (&raw_key)[N]) {
  return get_json_key_and_index(raw_key).first;
}

template <size_t N>
constexpr int extract_index(const char (&raw_key)[N]) {
  return get_json_key_and_index(raw_key).second;
}

// ---------------------------------------------------------------------------
//   is_generic_key / are_generic_keys
// ---------------------------------------------------------------------------

template <size_t N>
constexpr bool is_generic_key(const char (&raw_key)[N]) {
  for (size_t i = 0; i < N; i++) {
    if (raw_key[i] == JSON_ARRAY_START_CHARACTER) return false;
  }
  return true;
}

constexpr bool are_generic_keys() {
  return true;
}

template <typename Value>
constexpr bool are_generic_keys(Value&) { return false; }

template <typename Key, typename Value, typename... Rest>
constexpr bool are_generic_keys(const Key& key, Value&, Rest&&... rest) {
  return is_generic_key(key) && are_generic_keys(std::forward<Rest>(rest)...);
}

// ---------------------------------------------------------------------------
//   MultidimensionalArrayIndex
// ---------------------------------------------------------------------------

template<uint8_t N>
struct MultidimensionalArrayIndex {
  mutable int16_t _index[N];
  uint8_t _depth;

  constexpr MultidimensionalArrayIndex() : _index{0}, _depth(0) {
    JSON_DEBUG_COLOR(COLOR_RED, "MultidimensionalArrayIndex created\n");
  }

  void push(int16_t index = 0) {
    if (_depth >= N - 1) return;
    _index[++_depth] = index;
  }

  int16_t pop() {
    if (_depth == 0) return -1;
    return _index[--_depth];
  }

  void reset() {
    _depth = 0;
  }

  uint8_t getDepth() const {
    return _depth;
  }

  constexpr int16_t getIndex(int8_t depth) const {
    
    if (depth < 0) {
      depth = _depth + depth;
    }
    
    if (depth > _depth || depth < 0) return -1;
    
    return _index[static_cast<uint8_t>(depth)];
  }

  int16_t getIndex() const {
    return _index[_depth];
  }

  void setIndex(int16_t index) {
    if (_depth < N) {
      //DEBUG_PRINTF("setIndex %d at depth %d\n", index, _depth);
      _index[_depth] = index;
    }
  }
};
// ---------------------------------------------------------------------------
//   JSONKey
// ---------------------------------------------------------------------------

struct JSONKey {
  std::string_view _key;
  int16_t          _index;
  uint32_t         _hash;
  MultidimensionalArrayIndex<10> _array_index;

  constexpr JSONKey()
      : _key(""), _index(-1), _hash(0), _array_index() {}

  explicit constexpr JSONKey(int index)
      : _key(""), _index(index), _hash(static_cast<uint32_t>(index)),
        _array_index() {}

  // Constructeur depuis littéral — tout calculé à la compilation via
  // la liste d'initialisation (obligatoire en C++17 pour constexpr)
  template <size_t N>
  constexpr JSONKey(const char (&key)[N])
      : _key(extract_key(key)),
        _index(extract_index(key)),
        _hash(hash32(extract_key(key))),
        _array_index() {
    JSON_DEBUG_WARNING("Created key from const char [N] ");
#if JSON_DEBUG_LEVEL > 0
    this->print();
#endif
  }

  template <size_t N>
  bool operator==(const char (&key)[N]) const {
    return std::strncmp(key, _key.data(), N - 1) == 0;
  }

  bool operator==(const JSONKey& other) const { return _hash == other._hash; }

  constexpr bool operator==(const std::string_view& other_sv) const {
    return _key == other_sv;
  }
  
  template <size_t N>
  bool operator!=(const char (&key)[N]) const {
    return std::strncmp(key, _key.data(), N - 1) != 0;
  }
  
  bool operator!=(const JSONKey& other) const { return _hash != other._hash; }
  
  constexpr bool operator!=(const std::string_view& other_sv) const {
    return _key != other_sv;
  }

  constexpr int16_t operator[](int8_t index) const {
    return _array_index.getIndex(index);
  }

  constexpr operator uint32_t()         const { return _hash; }
  constexpr operator std::string_view() const { return _key; }

  size_t      length() const { return _key.length(); }
  const char* data()   const { return _key.data(); }

  void setKey(const std::string_view& key) {
    _key  = key;
    _hash = hash32(_key);
  }

  int16_t getIndex()       const { return _index; }
  void setIndex(int16_t i)       { _index = i; }

  int16_t getArrayIndex(int8_t depth)  const { return _array_index.getIndex(depth); }

  int16_t getArrayIndex()  const { return _array_index.getIndex(); }

  uint8_t getArrayIndexDepth() const { return _array_index.getDepth(); }

  void setArrayIndex(int16_t i) { _array_index.setIndex(i); }

  bool is_indexed() const { return _index >= 0; }

  void print() const {
    char array_index_buf[64];
    uint8_t len = _array_index.getDepth() + 1;
    size_t offset = 0;
    for(uint8_t i = 0; i < len; i++) {
      offset += snprintf(array_index_buf + offset, sizeof(array_index_buf) - offset, "%d ", _array_index.getIndex(i));
    }

    array_index_buf[offset] = '\0';
    
    DEBUG_PRINTF("JSONKey: %.*s index=%d hash=%u array_index=%s\n", (int)length(), data(), _index, _hash, array_index_buf);
  }
};
