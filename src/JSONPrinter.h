#pragma once

#include "demangled.h"
#include "types.h"

constexpr uint32_t get_last_bitwise_mask_index(uint32_t mask);

// ── Toutes les fonctions internes prennent Cursor par référence ──────────────
// Cela évite de copier StreamCursor (~280 octets avec RingBuffer<256>) à chaque
// niveau de récursion template, ce qui provoquait un stack overflow sur ESP8266
// (pile disponible ~3 Ko seulement).
//
// Seules les deux surcharges publiques JSON::print() reçoivent Cursor par valeur
// (interface inchangée pour l'appelant) et transmettent immédiatement une
// référence aux fonctions internes.
// ─────────────────────────────────────────────────────────────────────────────

template <typename Cursor, typename... Args> size_t print_to(Cursor &output, const char* format, Args &&...args);

template <typename Cursor, typename T, typename... Rest>
size_t constexpr print_key_value_pair(uint32_t mask, size_t idx, int &last_idx, Cursor &output, const char* key,
                                      T &value, Rest &&...rest);

template <typename Cursor, typename... Args> constexpr size_t print_json(uint32_t mask, Cursor &output, Args &&...args);

template <typename Cursor, typename T> size_t constexpr print_array_to(Cursor &output, T &array);

template <typename Cursor, typename T, size_t N> size_t constexpr print_hex_to(Cursor &output, T (&value)[N]);

template <typename Cursor, typename T, size_t N> size_t constexpr print_char_array_to(Cursor &output, T (&value)[N]);

template <typename Cursor, typename T> size_t constexpr print_value_to(Cursor &output, T &value);

template <typename Cursor> size_t print_object_pointer_to(Cursor &output, void *value);

namespace JSON {

// ── Surcharge pour StreamCursor / PointerCursorWriter ────────────────────────
template <typename Cursor, typename... Args>
enable_if_t<is_cursor_writer_v<Cursor> &&
                key_value_checker_v<parsed_types, arguments_types, arguments_array_types, Args...>,
            size_t>
_print(uint32_t mask, Cursor &output, Args &&...args) {
  return print_json(mask, output, std::forward<Args>(args)...);
}

// ── Surcharge pour dérivées de Stream ──────────────────────────────────────────
template <typename T, typename... Args>
std::enable_if_t<is_stream_v<T>, size_t> print(uint32_t mask, T &stream,
                                                                                           Args &&...args) {
  StreamCursor c(stream);
  return _print(mask, c, std::forward<Args>(args)...);
}

// ── Surcharge pour tampons char bruts (char[N] ou char*) ─────────────────────
template <typename Buffer, typename... Args>
std::enable_if_t<(std::is_array<std::remove_reference_t<Buffer>>::value ||
                  std::is_same<std::decay_t<Buffer>, char *>::value),
                 size_t>
print(uint32_t mask, Buffer &&buffer, Args &&...args) {
  if constexpr (std::is_array<std::remove_reference_t<Buffer>>::value) {
    constexpr size_t N = sizeof(std::remove_reference_t<Buffer>) / sizeof(char);
    PointerCursorWriter c(buffer, N);
    return _print(mask, c, std::forward<Args>(args)...);
  } else {
    PointerCursorWriter c(buffer, JSON::MAX_PRINTF_BUFFER_SIZE);
    return _print(mask, c, std::forward<Args>(args)...);
  }
}

} // namespace JSON

// ── Cas de base de la récursion (aucune paire restante) ───────────────────────
template <typename Cursor>
inline size_t constexpr print_key_value_pair(uint32_t /*mask*/, size_t /*idx*/, int & /*last_idx*/,
                                             Cursor & /*output*/) {
  return 0;
}

// ── Cas récursif ──────────────────────────────────────────────────────────────
template <typename Cursor, typename T, typename... Rest>
size_t constexpr print_key_value_pair(uint32_t mask, size_t idx, int &last_idx, Cursor &output, const char* key,
                                      T &value, Rest &&...rest) {
  size_t len = 0;

  if (mask == 0 || mask & (1 << idx)) {
    len += output.write("\"");
    len += output.write(key);
    len += output.write("\":");

    size_t vlen = print_value_to(output, value);
    len += vlen;

    if constexpr (sizeof...(Rest) > 0) {
      if (mask == 0 || idx < last_idx) {
        len += output.write(",");
      }
    }
  }

  len += print_key_value_pair(mask, ++idx, last_idx, output, std::forward<Rest>(rest)...);
  return len;
}

// ── print_value_to ────────────────────────────────────────────────────────────
template <typename Cursor, typename T> size_t constexpr print_value_to(Cursor &output, T &value) {

  if constexpr (std::is_same_v<remove_cvref_t<T>, bool>) {
    return print_to(output, "%s", value ? "true" : "false");
  } else if constexpr (is_char_array_v<T>) {
    return print_char_array_to(output, value);
  } else if constexpr (is_uint_array_v<T>) {
    if (JSON::PRINT_BUFFER_AS_HEX) {
      return print_hex_to(output, value);
    } else {
      return print_array_to(output, value);
    }
  } else if constexpr (is_container_v<T>) {
    return print_array_to(output, value);
  } else if constexpr (std::is_floating_point_v<remove_cvref_t<T>>) {
    return print_to(output, "%.15g", value);
    // } else if constexpr (std::is_unsigned_v<remove_cvref_t<T>>) {
    //   return print_to(output, "%u", value);
  } else if constexpr (std::is_integral_v<remove_cvref_t<T>>) {
    return print_to(output, "%lld", (long long)value);
  } else if constexpr (std::is_same_v<remove_cvref_t<T>, std::string_view>) {
    return print_to(output, "\"%.*s\"", (int)value.length(), value.data());
  } else if constexpr (std::is_base_of_v<JSONObject, remove_cvref_t<T>>) {
    return print_object_pointer_to(output, (void *)&value);
  } else if constexpr (std::is_pointer_v<T>) {
    if (value == nullptr) {
      return output.write("null");
    } else {
      if constexpr (std::is_base_of_v<JSONObject, remove_cvref_t<std::remove_pointer_t<T>>>) {
        return print_object_pointer_to(output, value);
      } else {
        return print_to(output, "%p", value);
      }
    }
  } else {
#if defined(__EXCEPTIONS) && defined(__GXX_RTTI)
    // static_assert(false, "cannot print type");
#endif
    JSON_DEBUG_TYPES("Cannot print type %s\n", value);
    return output.write("null");
  }
}

// ── print_object_pointer_to ───────────────────────────────────────────────────
template <typename Cursor> [[gnu::noinline]] size_t print_object_pointer_to(Cursor &output, void *value) {
#ifdef __EXCEPTIONS
  try {
#endif
    JSONObject *jsonObject = static_cast<JSONObject *>(value);
    return jsonObject->toJSON(output);
#ifdef __EXCEPTIONS
  } catch (const std::exception &e) {
    return output.write("null");
  }
#endif
}

// ── print_to ──────────────────────────────────────────────────────────────────
template <typename Cursor, typename... Args> size_t print_to(Cursor &output, const char* format, Args &&...args) {
  return output.printf(format, std::forward<Args>(args)...);
}

// ── Helpers array_size ────────────────────────────────────────────────────────
template <typename T, size_t N> size_t array_size(T (&array)[N]) { return N; }
template <typename T> size_t array_size(std::vector<T> &array) { return array.size(); }
template <typename T, size_t N> size_t array_size(std::array<T, N> &) { return N; }

// ── print_array_to ────────────────────────────────────────────────────────────
template <typename Cursor, typename T> size_t constexpr print_array_to(Cursor &output, T &array) {
  size_t N = array_size(array);
  size_t len = 0;

  len += output.write("[");

  for (size_t i = 0; i < N; i++) {
    size_t vlen = print_value_to(output, array[i]);
    len += vlen;
    if (i < N - 1) {
      len += output.write(",");
    }
  }

  len += output.write("]");
  return len;
}

// ── print_char_array_to ───────────────────────────────────────────────────────
template <typename Cursor, typename T, size_t N> size_t constexpr print_char_array_to(Cursor &output, T (&value)[N]) {
  return print_to(output, "\"%.*s\"", (int)(N - 1), value);
}

// ── print_hex_to ──────────────────────────────────────────────────────────────
template <typename Cursor, typename T, size_t N> size_t constexpr print_hex_to(Cursor &output, T (&value)[N]) {
  size_t hex_size = sizeof(T) * 2;
  size_t len = 0;

  len += output.write("\"");
  for (size_t i = 0; i < N; i++) {
    len += output.printf("%0.*X", int(hex_size), value[i]);
  }
  len += output.write("\"");

  return len;
}

// ── print_json ────────────────────────────────────────────────────────────────
template <typename Cursor, typename... Args>
constexpr size_t print_json(uint32_t mask, Cursor &output, Args &&...args) {
  static_assert(sizeof...(Args) % 2 == 0, "Le nombre d'arguments doit être pair");

  size_t len = 0;
  int last_index = get_last_bitwise_mask_index(mask);

  len += output.write("{");

  size_t inner = print_key_value_pair(mask, 0, last_index, output, std::forward<Args>(args)...);
  len += inner;

  len += output.write("}");
  return len;
}

// ── get_last_bitwise_mask_index ───────────────────────────────────────────────
constexpr uint32_t get_last_bitwise_mask_index(uint32_t mask) {
  if (mask == 0)
    return -1;

  uint32_t index = 0;

  while (mask >>= 1) {
    index++;
  }

  return index;
}
