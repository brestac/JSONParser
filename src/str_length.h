#pragma once

constexpr size_t str_length(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
    if (len >= 2048) break;
  }
  return len;
}
