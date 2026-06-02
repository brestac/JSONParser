#pragma once

#include <stddef.h>
#include "StreamCursor.h"
#include "PointerCursor.h"

NAMESPACE_JSON_BEGIN

template<typename Cursor> class StaticString {
public:
  //StaticString(Cursor& cursor, size_t length) : _cursor(cursor), _length(length) {}

  static void increase_pool_size(size_t size_needed) {
    size_t new_size = s_pool_size + size_needed;
    JSON_DEBUG_COLOR(COLOR_BLUE, "increase pool size to %zu octets\n", new_size);
    _set_pool_size(new_size);
  }

  static void ensure_pool_size(size_t input_size) {
    size_t new_size = s_pool_offset + input_size;
    JSON_DEBUG_COLOR(COLOR_BLUE, "Ensure pool size is at least %zu octets\n", new_size);
    _set_pool_size(new_size);
  }
  
  static void _set_pool_size(size_t new_size) {
    if (new_size <= s_pool_size) return;  // déjà suffisant
    
    char* p = static_cast<char*>(realloc(s_string_pool, new_size));

    if (p) {
      s_string_pool = p;
      s_pool_size   = new_size;
      JSON_DEBUG_COLOR(COLOR_BLUE, "Pool agrandi à %zu octets\n", new_size);
    } else {
      JSON_DEBUG_WARNING("Realloc échoué pour %zu octets\n", new_size);
    }
  }

  static void reset_offset() {
    s_pool_offset = 0;
    JSON_DEBUG_COLOR(COLOR_BLUE, "Pool offset réinitialisé\n");
  }

  static void clear() {
    if (s_string_pool) {
      free(s_string_pool);
      s_string_pool = nullptr;
      s_pool_size = 0;
      s_pool_offset = 0;
    }
    JSON_DEBUG_COLOR(COLOR_BLUE, "Pool détruit\n");
  }

  static bool write(unsigned char c) {
    if (s_pool_offset >= s_pool_size) {
      JSON_DEBUG_WARNING("Pool plein\n");
      return false;
    }

    s_string_pool[s_pool_offset++] = static_cast<char>(c);

    return true;
  }

  static char *current_pos() {
    return s_string_pool + s_pool_offset;
  }

  static char *data() {
    return s_string_pool;
  }

  static size_t offset() {
    return s_pool_offset;
  }

  static void print() {
    DEBUG_PRINTF("Pool: %zu octets, offset: %zu: '%.*s'\n", s_pool_size, s_pool_offset, (int)s_pool_offset, s_string_pool);
  }

  static std::string_view get_string_view(size_t offset, size_t length) {
    return std::string_view(s_string_pool + offset, length);
  }

private:
  static char*  s_string_pool;  // pointeur heap, nullptr jusqu'au premier appel
  static size_t s_pool_size;    // taille actuellement allouée
  static size_t s_pool_offset;  // offset courant dans le pool
};

template<typename Cursor> char* StaticString<Cursor>::s_string_pool = nullptr;
template<typename Cursor> size_t StaticString<Cursor>::s_pool_offset = 0;
template<typename Cursor> size_t StaticString<Cursor>::s_pool_size = 0;

// template<> char* StaticString<const PointerCursorReader>::s_string_pool = nullptr;
// template<> size_t StaticString<const PointerCursorReader>::s_pool_offset = 0;
// template<> size_t StaticString<const PointerCursorReader>::s_pool_size = 0;

NAMESPACE_JSON_END