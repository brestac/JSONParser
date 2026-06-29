#pragma once

#include <stddef.h>
#include <tuple>
#include <array>

#include "PointerCursor.h"
#include "StreamCursor.h"
#include "constants.h"
#include "macros.h"
#include "utils.h"

NAMESPACE_JSON_BEGIN

static std::array<void (*)(), 20> s_pool_deleters = {nullptr};
static size_t s_pool_count = 0;

static void clear_all() {
  JSON_DEBUG_COLOR(COLOR_RED, "clear_all %zu pools\n", s_pool_count);
    for(size_t i = 0; i < s_pool_count; i++) {
        auto fn = s_pool_deleters[i];
        if (fn) fn();
    }

    s_pool_count = 0;
    s_pool_deleters.fill(nullptr);
}

template <typename T, size_t N = 0> class StaticString {
  // struct Entries {
  //   uint32_t hash;
  //   size_t offset;
  // };

public:
  StaticString() = delete;
  StaticString(const StaticString &) = delete;
  StaticString &operator=(const StaticString &) = delete;
  StaticString(StaticString &&) = delete;
  StaticString &operator=(StaticString &&) = delete;
  ~StaticString() = delete;

  static void ensure_pool_size(size_t n) {
    JSON_DEBUG_COLOR(COLOR_CYAN, "StaticString<%s> ensure_pool_size %zu\n", typeid(T).name(), n);
    size_t new_size = s_pool_offset + n * MAX_VALUE_LENGTH;
    _set_pool_size(new_size);
  }

  static size_t register_if_needed() {
    static bool registered = false;
    
    if (registered) return s_pool_count;
  
    JSON_DEBUG_COLOR(COLOR_BLUE, "Registering StaticString<%s> #%zu\n", typeid(T).name(), s_pool_count);

    if (s_pool_count >= s_pool_deleters.size()) return false;
    s_pool_deleters[s_pool_count] = clear;
    registered = true;

    return s_pool_count++;
  }

  static void _set_pool_size(size_t new_size, bool allow_reduction = false) {
    
    if (allow_reduction == false && new_size <= s_pool_size)
      return; // déjà suffisant

    if (new_size > MAX_STRING_POOL_SIZE) {
      JSON_DEBUG_COLOR( COLOR_RED, "Pool trop grand: %zu octets demandés, max: %zu octets\n", new_size, MAX_STRING_POOL_SIZE);
      new_size = MAX_STRING_POOL_SIZE;
    }

    bool pool_exists = s_string_pool != nullptr;
    char *p = static_cast<char *>(realloc(s_string_pool, new_size));

    if (p) {
      JSON_DEBUG_COLOR(COLOR_BLUE, "StaticString<%s,%zu> %s à %zu octets\n", typeid(T).name(), N, (new_size > s_pool_size) ? "agrandi" : "réduit", new_size);
      
      if (pool_exists == false) {
        register_if_needed();
      }
      s_string_pool = p;
#ifdef JSON_DEBUG_MEM
      GLOBAL_STRING_POOL_SIZE += new_size - s_pool_size;
#endif
      s_pool_size = new_size;
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
    if (s_string_pool != nullptr) {
      free(s_string_pool);
#ifdef JSON_DEBUG_MEM
      GLOBAL_STRING_POOL_SIZE -= s_pool_size;
#endif
      s_string_pool = nullptr;
      s_pool_size = 0;
      s_pool_offset = 0;
      n_values = 0;
#ifdef JSON_DEBUG_MEM
#ifdef __GXX_RTTI
      JSON_DEBUG_COLOR(COLOR_RED, "Pool<%s> détruit\n", typeid(T).name());
#endif
      if (GLOBAL_STRING_POOL_SIZE == 0) {
        JSON_DEBUG_COLOR(COLOR_RED, "All pools destroyed\n");
      }
#endif
    } else {
#ifdef JSON_DEBUG_MEM
      JSON_DEBUG_COLOR(COLOR_RED, "Pool déjà détruit\n");
#endif
    }
  }
/*
  static bool write(unsigned char c) {
    if (s_pool_offset >= s_pool_size) {
      JSON_DEBUG_WARNING("Pool plein\n");
      return false;
    }

    s_string_pool[s_pool_offset++] = static_cast<char>(c);

    return true;
  }
*/
  // write at position n in the current offset without moving offest
  static bool write_at(unsigned char c, size_t n) {
    if (n >= s_pool_size) {
      JSON_DEBUG_WARNING("Pool plein\n");

      return false;
    }

    s_string_pool[s_pool_offset + n] = static_cast<char>(c);

    return true;
  }
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
/*
  static void get_static_buffer(const char *str, size_t len, const char *&output) {
    uint32_t hash = 0;

    if constexpr (N > 0) {
      hash = hash32(str, len);
      if (n_values >= MAX_KEY_VALUE_COUNT) {
        JSON_DEBUG_COLOR(COLOR_RED, "StaticString<%s> pool full\n", typeid(T).name());
        output = EMPTY_STRING;
      }

      int offset = find(hash);

      if (offset >= 0) {
        output = s_string_pool + offset;
        return;
      }
    }
    
    if (s_pool_offset + len > s_pool_size) {
      ensure_pool_size(1);
    }
    // si toujours insuffisant après realloc : erreur réelle, on tronque
    if (s_pool_offset + len > s_pool_size) {
      output = EMPTY_STRING;
      return;
    }

    if constexpr (N > 0) {
      s_entries[n_values] = {hash, s_pool_offset};
    }
    
    JSON_DEBUG_COLOR(COLOR_RED, "StaticString<%s,%zu> new entry for hash %u at offset %zu : '%.*s'\n", typeid(T).name(), N, hash, s_pool_offset, (int)len, str );
    char *dest = s_string_pool + s_pool_offset;
    strncpy(dest, str, len);
    dest[len] = '\0';
    s_pool_offset += len;
    n_values++;
    output = dest;
  }

  static int find(uint32_t hash) {
    for (size_t i = 0; i < n_values; i++) {
      if (s_entries[i].hash == hash) {
        JSON_DEBUG_COLOR(
            COLOR_GREEN,
            "StaticString<%s> found match for hash %u at offset %d\n",
            typeid(T).name(), hash, s_entries[i].offset);
        return static_cast<int>(s_entries[i].offset);
      }
    }
    JSON_DEBUG_COLOR(COLOR_RED, "StaticString<%s> no match for hash %u\n",
                     typeid(T).name(), hash);
    return -1;
  }
*/
  static char *current_pos() { return s_string_pool + s_pool_offset; }

  static char *data() { return s_string_pool; }

  static size_t offset() { return s_pool_offset; }

  static void increment_values_counter() { n_values++; }

  static void move_offset(int n) { 
    if (s_pool_offset + n > s_pool_size || s_pool_offset + n < 0) {
      JSON_DEBUG_COLOR(COLOR_RED, "StaticString<%s> move_offset out of bounds\n", typeid(T).name());
      return;
    }

    s_pool_offset += n;
  }

  static void print() {
    JSON_DEBUG_COLOR(COLOR_BLACK, "StaticString<%s> pool: %zu octets, offset: %zu, values:%u, " "content: '%.*s'\n", typeid(T).name(), s_pool_size, s_pool_offset, n_values, (int)s_pool_offset, s_string_pool);
  }

private:
  inline static char *s_string_pool = nullptr; // pointeur heap, nullptr jusqu'au premier appel
  inline static size_t s_pool_size = 0;        // taille actuellement allouée
  inline static size_t s_pool_offset = 0;      // offset courant dans le pool
  inline static size_t n_values = 0;           // nombre de valeurs stockées
  // static Entries s_entries[];
};

NAMESPACE_JSON_END