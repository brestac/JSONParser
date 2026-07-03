#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// include for ntohs
#ifdef ARDUINO_ARCH_ESP8266 // TODO: check for other platforms
#include <ESP8266WiFi.h>
#else
#include <arpa/inet.h>
#endif

#include "constants.h"
#include "macros.h"
#include "CursorScanner.h"

template <typename T, size_t N> constexpr bool copy_bytes_be_to_h(T (&dst)[N], uint8_t *src, size_t src_size);
template <typename T, size_t N> constexpr bool copy_bytes_be_to_h(T dst, uint8_t (&src)[N]);
template <typename T, size_t N> constexpr bool copy_hex_be_to_h(T (&dst)[N], const char* src, size_t src_size);
bool get_byte_fromHexString(uint8_t &value, const char* src, size_t src_size);
template <typename T> bool get_unsigned_integral_fromHexString(T &value, const char* src, size_t src_size);
template <typename T> constexpr T be_to_h(T value);
unsigned long long now();

template<uint8_t N>
constexpr bool is_in_ranges(char c, char (&ranges)[N][2]) {
  for (uint8_t i = 0; i < N; i++) {
    if (c >= ranges[i][0] && c <= ranges[i][1]) {
      return true;
    }
  }

  return false;
}

constexpr uint8_t _hex_to_dec(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return 0;
  }
}

template <typename T> constexpr T be_to_h(T value) {
  if constexpr (sizeof(T) == 4) {
    return ntohl(value);
  } else if constexpr (sizeof(T) == 2) {
    return ntohs(value);
  } else {
    return value;
  }
}
/*
template <typename T, size_t N> constexpr bool copy_array(T (&dst)[N], T (&src)[N]) {
  bool modified = false;

  for (size_t i = 0; i < N; i++) {
    T new_value = src[i];

    if (dst[i] != new_value) {
      dst[i] = new_value;
      modified = true;
    }
  }

  return modified;
}

template <typename T, size_t N, size_t M> constexpr bool copy_array(T (&dst)[N][M], T (&src)[N][M]) {
  bool modified = false;

  for (size_t i = 0; i < N; i++) {
    modified = copy_array(dst[i], src[i]);
  }

  return modified;
}
*/
constexpr bool _is_hex_char(char c) {
  return is_in_ranges(c, JSON_HEX_CHARACTERS_RANGES);
}

bool get_byte_fromHexString(uint8_t &value, const char* src, size_t src_size) {
  if (src_size < 2) return false;

  if (_is_hex_char(src[0])) {
    uint8_t high_mid_byte = _hex_to_dec(src[0]);
    if (_is_hex_char(src[1])) {
      uint8_t low_mid_byte = _hex_to_dec(src[1]);
      value = (high_mid_byte << 4) | low_mid_byte;
      return true;
    }
  }

  return false;
}

template <typename T> bool get_unsigned_integral_fromHexString(T &value, const char* src, size_t src_size) {
  constexpr size_t target_length = sizeof(T);

  if (src_size < target_length * 2) {
    return false;
  }

  uint8_t bytes[target_length];
  for (size_t i = 0; i < target_length; i++) {
    if (!get_byte_fromHexString(bytes[i], src + i * 2, src_size - i * 2)) {
      return false;
    }
  }

  value = be_to_h(*(T *)bytes);

  return true;
}

template <typename T, size_t N> constexpr bool copy_hex_be_to_h(T (&dst)[N], const char* src, size_t src_size) {

  size_t dst_element_size = sizeof(T);
  size_t dst_elements_count = N;
  size_t dst_size = dst_element_size * dst_elements_count;

  if (dst_size == 0 || src_size == 0) {
    return false;
  }

  bool modified = false;

  size_t max_elements_count = std::min(dst_elements_count, src_size / dst_element_size);

  for (size_t i = 0; i < max_elements_count; i++) {
    T new_element_value = 0;
    if (get_unsigned_integral_fromHexString(new_element_value, src + i * dst_element_size * 2,
                                            src_size - i * dst_element_size * 2)) {
      if (dst[i] != new_element_value) {
        dst[i] = new_element_value;
        modified = true;
      }
    }
  }

  size_t dst_final_size = max_elements_count * dst_element_size;

  if (dst_final_size > src_size) {
    memset((uint8_t *)dst + dst_final_size, 0, dst_final_size - src_size);
    modified = true;
  }

  return modified;
}

void print_bitwise_mask(size_t mask, size_t count) {

  DEBUG_PRINTF("mask: ");

  for (size_t i = 0; i < count; i++) {
    DEBUG_PRINTF("%d ", (mask & (1 << i)) != 0);
  }

  DEBUG_PRINTF("\n");
}

unsigned long long now() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}
#if DEBUG_LEVEL > 0
void replace_endl(char *str, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (str[i] == '\n' || str[i] == '\r') {
      str[i] = ' ';
    }
  }
}

template <size_t N, size_t M> void replace_str(char (&input)[N], char (&oldChars)[M], char newChar) {
  for (size_t i = 0; i < N; i++) {
    for (size_t j = 0; j < M; j++) {
      if (input[i] == oldChars[j]) {
        input[i] = newChar;
      }
    }
  }
}
#endif
// std::string_view copy_to_sv(const char* str, size_t len) {
//   static char buffer[JSON::MAX_KEY_LENGTH];
//   strncpy(buffer, str, len);
//   buffer[len] = '\0';
//   return std::string_view(buffer, len);
// }

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