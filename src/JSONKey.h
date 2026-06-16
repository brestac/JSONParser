#pragma once

#include <string_view>

#include "StreamScanner.h"
#include "str_length.h"
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

constexpr int parse_int_constexpr(const char* str) {
  int result = 0;
  size_t iteration = 0;
  while (*str >= '0' && *str <= '9' && ++iteration < JSON::MAX_ITERATIONS) {
    result = result * 10 + (*str - '0');
    ++str;
  }
  return result;
}

// ---------------------------------------------------------------------------
//   get_json_key_and_index — constexpr, sans strtol
//   Retourne {string_view sur la clé, index ou -1}
// ---------------------------------------------------------------------------

template <size_t N>
constexpr std::pair<std::string_view, int> get_json_key_and_index(
    const char (&raw_key)[N]) {
  for (size_t i = 0; i < N; ++i) {
    if (raw_key[i] == '[') {
      return { std::string_view(raw_key, i),
               parse_int_constexpr(raw_key + i + 1) };
    }
    if (raw_key[i] == '\0') break;
  }
  return { std::string_view(raw_key, N - 1), -1 };
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
//   JSONKey
// ---------------------------------------------------------------------------

struct JSONKey {
  std::string_view _key;
  int              _index;
  uint32_t         _hash;
  int              _array_index;

  constexpr JSONKey()
      : _key(""), _index(-1), _hash(0), _array_index(-1) {}

  explicit constexpr JSONKey(int index)
      : _key(""), _index(index), _hash(static_cast<uint32_t>(index)),
        _array_index(-1) {}

  // Constructeur depuis littéral — tout calculé à la compilation via
  // la liste d'initialisation (obligatoire en C++17 pour constexpr)
  template <size_t N>
  constexpr JSONKey(const char (&key)[N])
      : _key(extract_key(key)),
        _index(extract_index(key)),
        _hash(hash32(extract_key(key))),
        _array_index(-1) {
    JSON_DEBUG_WARNING("Created key %.*s index=%d from const char [N]\n",
                       (int)length(), data(), _index);
  }

  template <size_t N>
  bool operator==(const char (&key)[N]) const {
    return std::strncmp(key, _key.data(), N - 1) == 0;
  }

  bool operator==(const JSONKey& other) const { return _hash == other._hash; }

  constexpr bool operator==(const std::string_view& other_sv) const {
    return _key == other_sv;
  }

  constexpr operator uint32_t()         const { return _hash; }
  constexpr operator std::string_view() const { return _key; }

  size_t      length() const { return _key.length(); }
  const char* data()   const { return _key.data(); }

  void setKey(const std::string_view& key) {
    _key  = key;
    _hash = hash32(_key);
  }

  int  getIndex()       const { return _index; }
  void setIndex(int i)        { _index = i; }

  int  getArrayIndex()  const { return _array_index; }
  void setArrayIndex(int i) {
    _array_index = i;
    JSON_DEBUG_INFO("JSONKey setArrayIndex %d\n", i);
  }

  bool is_indexed() const { return _index >= 0; }
};
