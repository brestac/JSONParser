#pragma once

#include <string_view>

#include "utils.h"

// ---------------------------------------------------------------------------
//   hash32 — FNV-1a 32 bits, constexpr
// ---------------------------------------------------------------------------

constexpr uint32_t hash32(const char* str, size_t len) {
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

constexpr uint32_t operator""_hash(const char* str, size_t len) {
  return hash32(str, len);
}

// ---------------------------------------------------------------------------
//   MultidimensionalArrayIndex
// ---------------------------------------------------------------------------

template<uint8_t N>
struct MultidimensionalArrayIndex {
  mutable int16_t _index[N];
  uint8_t _depth;

  MultidimensionalArrayIndex() : _index{0}, _depth(0) {
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

  int16_t getIndex(int8_t depth) const {
    
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
  // int16_t          _index;
  uint32_t         _hash;
  MultidimensionalArrayIndex<10> _array_index;

  JSONKey() : _key(""), /*_index(-1),*/ _hash(0), _array_index() {}

  JSONKey(std::string_view key) : _key(key), /*_index(-1),*/ _hash(hash32(_key)), _array_index() {
    _array_index.reset();
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

  bool operator==(const std::string_view& other_sv) const {
    return _key == other_sv;
  }
  
  template <size_t N>
  bool operator!=(const char (&key)[N]) const {
    return std::strncmp(key, _key.data(), N - 1) != 0;
  }
  
  bool operator!=(const JSONKey& other) const { return _hash != other._hash; }
  
  bool operator!=(const std::string_view& other_sv) const {
    return _key != other_sv;
  }

  int16_t operator[](int8_t index) const {
    return _array_index.getIndex(index);
  }

  operator uint32_t()         const { return _hash; }
  operator std::string_view() const { return _key; }

  size_t      length() const { return _key.length(); }
  const char* data()   const { return _key.data(); }

  void setKey(const std::string_view& key) {
    _key  = key;
    _hash = hash32(_key);
  }

  // int16_t getIndex()       const { return _index; }
  // void setIndex(int16_t i)       { _index = i; }

  int16_t getArrayIndex(int8_t depth)  const { return _array_index.getIndex(depth); }

  int16_t getArrayIndex()  const { return _array_index.getIndex(); }

  uint8_t getArrayIndexDepth() const { return _array_index.getDepth(); }

  void setArrayIndex(int16_t i) { _array_index.setIndex(i); }

  //bool is_indexed() const { return _index >= 0; }

#if JSON_DEBUG_LEVEL > 0
  void print() const {
    char array_index_buf[64];
    uint8_t len = _array_index.getDepth() + 1;
    size_t offset = 0;
    for(uint8_t i = 0; i < len; i++) {
      offset += snprintf(array_index_buf + offset, sizeof(array_index_buf) - offset, "%d ", _array_index.getIndex(i));
    }

    array_index_buf[offset] = '\0';
    
    DEBUG_PRINTF("JSONKey: %.*s hash=%u array_index=%s\n", (int)length(), data(), _hash, array_index_buf);
  }
#endif
};
