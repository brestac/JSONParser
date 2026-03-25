#pragma once

#include "constants.h"
#include "macros.h"
#include "str_length.h"

struct JSONString {
  size_t size;
  char *buffer;
  size_t position;

  constexpr JSONString() : size(0), buffer(nullptr), position(0) {}
  constexpr JSONString(char *buffer, size_t size) : size(size), buffer(buffer), position(0) {}
  constexpr JSONString(const char *buffer, size_t size) : size(size), buffer((char *)buffer), position(0) {}
  constexpr JSONString(const char *buffer) : size(str_length(buffer)), buffer((char *)buffer), position(0) {
    JSON_DEBUG_INFO("JSONString created with size=%zu\n", size);
  }
  constexpr JSONString(std::string_view sv) : size(sv.length()), buffer((char *)sv.data()), position(0) {} 
  template <size_t N> constexpr JSONString(char (&buffer)[N]) : size(N - 1), buffer(buffer), position(0) {}
  template <size_t N> constexpr JSONString(const char (buffer)[N]) : size(N - 1), buffer(buffer), position(0) {}

  ~JSONString() {
    if (buffer != nullptr && position > 0) {
      buffer[position] = '\0';
      //JSON_DEBUG_INFO("JSONString destructor ended, buffer=%s\n", buffer);
    }
    size = 0;
    position = 0;
  }

  template <typename... Args>
  void constexpr write(const char *format, Args &&...args);
};
