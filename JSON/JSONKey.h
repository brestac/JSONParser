#pragma once

#include "JSONStreamParser.h"
#include "str_length.h"
// ---------------------------------------------------------------------------
//   JSONKey
// ---------------------------------------------------------------------------

std::string_view constexpr get_json_key(const char *raw_key, size_t key_len) {
  JSON::PointerCursor key_cursor(raw_key, key_len);
  const char *key_start = key_cursor.ptr();
  
  if (cursor_scan_ranges(key_cursor, JSON_KEY_CHARACTERS, key_len, true)) {
    return std::string_view(key_start, key_cursor.ptr() - key_start);
  }

  return std::string_view("");
}

int constexpr get_json_key_index(const char *raw_key, size_t key_len) {
  JSON::PointerCursor key_cursor(raw_key, key_len);
  
  if (cursor_scan_ranges(key_cursor, JSON_KEY_CHARACTERS, true)) {
    if (cursor_scan_char(key_cursor, JSON_ARRAY_START_CHARACTER, true)) {
      char *end = nullptr;
      int idx = std::strtol(raw_key, &end, 10);
      
      if (end != raw_key) {
        key_cursor.advance_to(end);
        if (cursor_scan_char(key_cursor, JSON_ARRAY_END_CHARACTER, true) && idx >= 0) {
          return idx;
        }
      }
    }
  }

  return -1;
}

bool constexpr is_key(const char *raw_key) {
  return (get_json_key(raw_key, str_length(raw_key)).length() > 0) && (get_json_key_index(raw_key, str_length(raw_key)) == -1);
}

inline bool are_generic_keys() {
  return true;
}

template <typename Value>
constexpr bool are_generic_keys(Value) {
  return false;
}

template <typename Key, typename Value, typename... Rest>
constexpr bool are_generic_keys(Key key, Value value, Rest... rest) {
  return (is_key(key)) && (are_generic_keys(rest...));
}

constexpr uint32_t hash32(const char *str, size_t len) {
  if (str == nullptr) return 0;

  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; ++i) {
    hash ^= static_cast<uint32_t>(str[i]);
    hash *= 16777619u;
  }
  return hash;
}

constexpr uint32_t hash32(std::string_view key) {
  return hash32(key.data(), key.length());  
}

// Opérateur littéral
constexpr uint32_t operator""_hash(const char *str, size_t len) {
  return hash32(str, len);
}

struct JSONKey {
  std::string_view _key;
  int _index;
  uint32_t _hash;
  int _array_index;

  constexpr JSONKey() : _key(""), _index(-1), _hash(0), _array_index(-1) {}

  constexpr JSONKey(int index) : _key(""), _index(index), _hash(index), _array_index(-1) {}

  constexpr JSONKey(const char *key, size_t len) : _key(get_json_key(key, len)), _index(get_json_key_index(key, len)), _hash(hash32(_key)), _array_index(-1) {
    JSON_DEBUG_INFO("Created key %.*s index=%d\n", (int)length(), data(), _index);
  }

  constexpr JSONKey(const char *key) : JSONKey(key, str_length(key)) {}

  template <size_t N>
  constexpr JSONKey(const char (&key)[N]) : JSONKey((const char *)key, N - 1) {}

  bool operator==(const JSONKey &other) const { return _hash == other._hash; }

  constexpr operator uint32_t() const { return _hash; }

  constexpr operator std::string_view() const { return _key; }

  size_t length() const { return _key.length(); }

  const char *data() const { return _key.data(); }

  void setKey(const char *key, size_t len) {
    _key = get_json_key(key, len);
    _index = get_json_key_index(key, len);
    _hash = hash32(_key);
    JSON_DEBUG_INFO("JSONKey setKey %.*s index=%d\n", (int)length(), data(), _index);
  }

  void setKey(std::string_view key) {
    setKey(key.data(), key.length());
  }

  int getIndex() const { return _index; }

  void setIndex(int index) { _index = index; }

  int getArrayIndex() const { return _array_index; }

  void setArrayIndex(int index) {
    _array_index = index;
    JSON_DEBUG_INFO("JSONKey setArrayIndex %d\n", index);
  }

  bool is_indexed() const { return _index >= 0; }
};
