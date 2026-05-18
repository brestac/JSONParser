#pragma once
#include <limits>
#include "constants.h"

//constexpr uint32_t max_len = std::numeric_limits<uint32_t>::max(); // Equals to 4294967295 bytes (4GB)

constexpr size_t str_length(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
    if (len >= JSON::MAX_POINTER_CURSOR_SIZE) break;
  }
  return len;
}
