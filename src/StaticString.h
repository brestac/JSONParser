#pragma once

#include <stddef.h>
#include "StreamCursor.h"
#include "PointerCursor.h"

NAMESPACE_JSON_BEGIN

template<typename Cursor> class StaticString {
public:
  //StaticString(Cursor& cursor, size_t length) : _cursor(cursor), _length(length) {}

  static void increase_pool_size(size_t size_needed) {
    size_t saved_space = n_values * MAX_VALUE_LENGTH - s_pool_offset;
    JSON_DEBUG_COLOR(COLOR_BLUE, "Saved space: %zu octets\n", saved_space);
    size_t new_size = std::max(int(s_pool_size - saved_space + size_needed), 0);
    JSON_DEBUG_COLOR(COLOR_BLUE, "Request to increase pool size to %zu octets\n", new_size);
    if (new_size > 0)
      _set_pool_size(new_size);
  }

  static void ensure_pool_size(size_t input_size) {
    size_t saved_space = n_values * MAX_VALUE_LENGTH - s_pool_offset;
    size_t new_size = s_pool_offset - saved_space + input_size;
    JSON_DEBUG_COLOR(COLOR_BLUE, "Request to ensure pool size is at least %zu octets\n", new_size);
    _set_pool_size(new_size);
  }
  
  static void _set_pool_size(size_t new_size, bool allow_reduction = false) {
    if (allow_reduction == false && new_size <= s_pool_size) return;  // déjà suffisant

    if (new_size > MAX_STRING_POOL_SIZE) {
      JSON_DEBUG_COLOR(COLOR_RED, "Pool trop grand: %zu octets demandés, max: %zu octets\n", new_size, MAX_STRING_POOL_SIZE);
      new_size = MAX_STRING_POOL_SIZE;
    }
    
    char* p = static_cast<char*>(realloc(s_string_pool, new_size));

    if (p) {
      JSON_DEBUG_COLOR(COLOR_BLUE, "Pool %s à %zu octets\n", (new_size > s_pool_size) ? "agrandi" : "réduit", new_size);
      s_string_pool = p;
      s_pool_size   = new_size;
    } else {
      JSON_DEBUG_COLOR(COLOR_RED, "Realloc échoué pour %zu octets\n", new_size);
    }
  }

  static void fit() {
    _set_pool_size(s_pool_offset + 1, true);
    JSON_DEBUG_COLOR(COLOR_BLUE, "Pool réduit à %zu octets\n", s_pool_size);
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

  static void increment_values_counter() {
    n_values++;
  }

  static void print() {
    DEBUG_PRINTF("StaticString pool: %zu octets, offset: %zu, values:%u, content: '%.*s'\n", s_pool_size, s_pool_offset, n_values, (int)s_pool_offset, s_string_pool);
  }

private:
  static char*  s_string_pool;  // pointeur heap, nullptr jusqu'au premier appel
  static size_t s_pool_size;    // taille actuellement allouée
  static size_t s_pool_offset;  // offset courant dans le pool
  static size_t n_values;      // nombre de valeurs stockées
};

template<typename Cursor> char* StaticString<Cursor>::s_string_pool = nullptr;
template<typename Cursor> size_t StaticString<Cursor>::s_pool_offset = 0;
template<typename Cursor> size_t StaticString<Cursor>::s_pool_size = 0;
template<typename Cursor> size_t StaticString<Cursor>::n_values = 0;

NAMESPACE_JSON_END