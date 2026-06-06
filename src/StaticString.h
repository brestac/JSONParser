#pragma once

#include <stddef.h>

#include "PointerCursor.h"
#include "StreamCursor.h"
#include "constants.h"
#include "macros.h"
#include "utils.h"

NAMESPACE_JSON_BEGIN
// static size_t POOL_N = 0;
static const char *EMPTY_STRING = "";

template <typename T> class StaticString {
  // struct Entries {
  //   uint32_t hash;
  //   size_t offset;
  //   size_t length;

  //   void print() {
  //     // PRINTF_COLOR(COLOR_RED, "  hash %u offset %zu value:%.*s\n", hash,
  //     offset,
  //                  (int)length, s_string_pool + offset);
  //   }
  // };

public:
  StaticString() = delete;
  StaticString(const StaticString &) = delete;
  StaticString &operator=(const StaticString &) = delete;

  static void increase_pool_size(size_t size_needed) {
    //size_t saved_space = n_values * MAX_VALUE_LENGTH - s_pool_offset;
    size_t new_size = s_pool_size + size_needed;
    _set_pool_size(new_size);
  }

  // static void ensure_pool_size(size_t input_size) {
  //   size_t saved_space = n_values * MAX_VALUE_LENGTH - s_pool_offset;
  //   size_t new_size = std::max(int(s_pool_offset + input_size - saved_space),
  //   0); JSON_DEBUG_COLOR(COLOR_BLUE,
  //                    "Request to ensure pool size is at least %zu octets\n",
  //                    new_size);
  //   _set_pool_size(new_size);
  // }

  static void _set_pool_size(size_t new_size, bool allow_reduction = false) {
    if (allow_reduction == false && new_size <= s_pool_size)
      return; // déjà suffisant

    if (new_size > MAX_STRING_POOL_SIZE) {
      //JSON_DEBUG_COLOR( COLOR_RED, "Pool trop grand: %zu octets demandés, max: %zu octets\n", new_size, MAX_STRING_POOL_SIZE);
      new_size = MAX_STRING_POOL_SIZE;
    }

    char *p = static_cast<char *>(realloc(s_string_pool, new_size));

    if (p) {
      // PRINTF_COLOR(COLOR_BLUE, "StaticString<%s> %s à %zu octets\n", typeid(T).name(), (new_size > s_pool_size) ? "agrandi" : "réduit", new_size);
      s_string_pool = p;
      s_pool_size = new_size;
    } else {
      JSON_DEBUG_COLOR(COLOR_RED, "Realloc échoué pour %zu octets\n", new_size);
    }
  }

  // static void fit() {
  //   _set_pool_size(s_pool_offset + 1, true);
  //   JSON_DEBUG_COLOR(COLOR_BLUE, "Pool réduit à %zu octets\n", s_pool_size);
  // }

  // static void reset_offset() {
  //   s_pool_offset = 0;
  //   JSON_DEBUG_COLOR(COLOR_BLUE, "Pool offset réinitialisé\n");
  // }

  // static void clear() {
  //   if (s_string_pool) {
  //     free(s_string_pool);
  //     s_string_pool = nullptr;
  //     s_pool_size = 0;
  //     s_pool_offset = 0;
  //   }
  //   JSON_DEBUG_COLOR(COLOR_BLUE, "Pool détruit\n");
  // }

  // static bool write(unsigned char c) {
  //   if (s_pool_offset >= s_pool_size) {
  //     JSON_DEBUG_WARNING("Pool plein\n");
  //     return false;
  //   }

  //   s_string_pool[s_pool_offset++] = static_cast<char>(c);

  //   return true;
  // }

  // static bool write(const char *str, size_t len) {
  //   if (s_pool_offset + len >= s_pool_size) {
  //     JSON_DEBUG_WARNING("Pool plein\n");
  //     return false;
  //   }

  //   strncpy(s_string_pool + s_pool_offset, str, len);
  //   uint32_t hash = hash32(str, len);
  //   s_entries[++n_values] = {hash, s_pool_offset};
  //   s_pool_offset += len;
  // }

static void get_static_buffer(const char *str, size_t len, const char *&output) {
    if (s_pool_offset + len > s_pool_size) {
        increase_pool_size(len);
    }
    // si toujours insuffisant après realloc : erreur réelle, on tronque
    if (s_pool_offset + len > s_pool_size) {
        output = EMPTY_STRING;
    } else {
      char *dest = s_string_pool + s_pool_offset;
      strncpy(dest, str, len);
      s_pool_offset += len;
      n_values++;
      output = dest;
    }
}

  /*
    static int find(uint32_t hash) {
      for (size_t i = 0; i < n_values; i++) {
        if (s_entries[i].hash == hash) {
          PRINTF_COLOR(COLOR_GREEN,
                       "StaticString<%s> found match for hash %u at offset %d :
    '%.*s'\n", typeid(T).name(), hash, s_entries[i].offset,
                       (int)s_entries[i].length,
                       s_string_pool + s_entries[i].offset);
          return static_cast<int>(s_entries[i].offset);
        }
      }

      return -1;
    }
  */

  static char *current_pos() { return s_string_pool + s_pool_offset; }

  static char *data() { return s_string_pool; }

  static size_t offset() { return s_pool_offset; }

  static void increment_values_counter() { n_values++; }

  static void print() {
    // PRINTF_COLOR(COLOR_BLACK, "StaticString<%s> pool: %zu octets, offset: %zu, values:%u, " "content: '%.*s'\n", typeid(T).name(), s_pool_size, s_pool_offset, n_values, (int)s_pool_offset, s_string_pool);
    // PRINTF_COLOR(COLOR_BLACK,
    //              "StaticString<%s> %zu: %zu octets, offset: %zu,
    //              values:%u\n", typeid(T).name(), s_index, s_pool_size,
    //              s_pool_offset, n_values);
    // for (size_t i = 0; i < n_values; i++) {
    //   s_entries[i].print();
    // }
  }

private:
  static char *s_string_pool;  // pointeur heap, nullptr jusqu'au premier appel
  static size_t s_pool_size;   // taille actuellement allouée
  static size_t s_pool_offset; // offset courant dans le pool
  static size_t n_values;      // nombre de valeurs stockées
  // static Entries s_entries[];
  // static size_t s_index;
};

template <typename T> char *StaticString<T>::s_string_pool = nullptr;
template <typename T> size_t StaticString<T>::s_pool_offset = 0;
template <typename T> size_t StaticString<T>::s_pool_size = 0;
template <typename T> size_t StaticString<T>::n_values = 0;
// template <typename T>
// typename StaticString<T>::Entries
//     StaticString<T>::s_entries[MAX_KEY_VALUE_COUNT];
// template <typename T> size_t StaticString<T>::s_index = POOL_N++;
NAMESPACE_JSON_END