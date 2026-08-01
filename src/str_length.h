#pragma once
#include <stddef.h>

constexpr size_t str_length(const char* str, uint32_t max_len) {
  uint32_t len = 0;
  while (len < max_len && str[len] != '\0') {
    ++len;
  }

  return len;
}
