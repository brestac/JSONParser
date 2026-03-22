#pragma once

#ifdef __EXCEPTIONS
#include <stdexcept>
#endif
#include <string_view>
#include <type_traits>

#include "macros.h"
#include "types.h"
#include "utils.h"
#include "JSONData.h"
#include "str_length.h"

constexpr int get_last_bitwise_mask_index(uint32_t mask);

template <typename Cursor, typename... Args>
std::enable_if_t<is_cursor_writer_v<Cursor>, size_t>
constexpr print_to(Cursor output, const char *format, Args &&...args);

template <typename Cursor, typename T, typename... Rest>
size_t constexpr print_key_value_pair(uint32_t mask, size_t idx, int &last_idx, Cursor output, const char *key, T &value, Rest &&...rest);

template <typename Cursor, typename... Args>
std::enable_if_t<key_value_checker_v<parsed_types, arguments_types, arguments_array_types, Args...>, size_t>
constexpr print_json(uint32_t mask, Cursor output, Args &&...args);

template <typename Cursor, typename T>
size_t constexpr print_array_to(Cursor output, T &array);

template <typename Cursor, typename T, size_t N>
size_t constexpr print_hex_to(Cursor output, T (&value)[N]);

template <typename Cursor, typename T>
size_t constexpr print_value_to(Cursor output, T &value);

template <typename Cursor>
size_t print_object_pointer_to(Cursor output, void *value);

namespace JSON {
  template <typename Cursor, typename... Args>
  enable_if_t<key_value_checker_v<parsed_types, arguments_types, arguments_array_types, Args...>, size_t>   
  print(uint32_t mask, Cursor output, Args &&...args) {
    return print_json(mask, output, std::forward<Args>(args)...);
  }
} // namespace JSON

template <typename Cursor>
inline size_t constexpr print_key_value_pair(uint32_t mask, size_t idx,
                                           int &last_idx, Cursor output) { return 0; }

template <typename Cursor, typename T, typename... Rest>
size_t constexpr print_key_value_pair(uint32_t mask, size_t idx, int &last_idx, Cursor output, const char *key, T &value,
                                    Rest &&...rest) {
  size_t len = 0;
  
  if (mask == 0 || mask & (1 << idx)) {
    len += output.write("\"");
    len += output.write(key);
    len += output.write("\":"); 
    len += print_value_to(output, value);

    if constexpr (sizeof...(Rest) > 0) {
      if (mask == 0 || idx < last_idx) {
        len += output.write(",");
      }
    }
  }
  // print_demangled_type("print_key_value_pair type: %s\n", value);

  len += print_key_value_pair(mask, ++idx, last_idx, output,
                       std::forward<Rest>(rest)...);
  return len;
}

template <typename Cursor, typename T>
size_t constexpr print_value_to(Cursor output, T &value) {
  if constexpr (is_char_array_v<T>) {
    return print_to(output, "\"%s\"", value);
  } else if constexpr (is_uint_array_v<T>) {
    if (JSON::PRINT_BUFFER_AS_HEX) {
      return print_hex_to(output, value);
    } else {
      return print_array_to(output, value);
    }
  } else if constexpr (is_container_v<T>) {
    return print_array_to(output, value);
  } else if constexpr (std::is_same_v<remove_cvref_t<T>, bool>) {
    return print_to(output, "%s", value ? "true" : "false");
  } else if constexpr (std::is_floating_point_v<remove_cvref_t<T>>) {
    return print_to(output, "%.15g", value);
  // } else if constexpr (std::is_unsigned_v<remove_cvref_t<T>>) {
  //   print_to(output, "%u", value);
  } else if constexpr (std::is_integral_v<remove_cvref_t<T>>) {
    return print_to(output, "%lld", (long long)value);
  } else if constexpr (std::is_same_v<remove_cvref_t<T>, std::string_view>) {
    return print_to(output, "\"%.*s\"", (int)value.length(), value.data());
   } else if constexpr (std::is_base_of_v<JSONData, remove_cvref_t<T>>) {
    return print_object_pointer_to(output, (void *)&value);
   } else if constexpr (std::is_pointer_v<T>) {
     if (value == nullptr) {
       return output.write("null");
     } else {
       if constexpr (std::is_base_of_v<JSONData, remove_cvref_t<std::remove_pointer_t<T>>>) {
         return print_object_pointer_to(output, value);       
       } else {
         return print_to(output, "%p", value);
       }
    }
  } else {
#if defined(__EXCEPTIONS) && defined(__GXX_RTTI)
    //static_assert(false, "In file " __FILE__ " line cannot print type " typeid(value).name());
#endif
    JSON_DEBUG_TYPES("Cannot print type %s\n", value);
    return output.write("null");
  }
}

template <typename Cursor>
[[gnu::noinline]] size_t print_object_pointer_to(Cursor output, void *value) {
#ifdef __EXCEPTIONS
  try {
#endif
    JSONData *jsonData = static_cast<JSONData *>(value);
    return jsonData->toJSON(output);
#ifdef __EXCEPTIONS
  }
  catch (const std::exception &e) {
    return output.write("null");
  }
#endif
}

template <typename Cursor, typename... Args>
std::enable_if_t<is_cursor_writer_v<Cursor>, size_t>
constexpr print_to(Cursor output, const char *format, Args &&...args) {
  return output.printf(format, std::forward<Args>(args)...);
}

template <typename T, size_t N>
size_t array_size(T (&array)[N]) {
  return N;
}

template <typename T>
size_t array_size(std::vector<T> &array) {
  return array.size();
}

template <typename T, size_t N>
size_t array_size(std::array<T, N> &array) {
  return N;
}

template <typename Cursor, typename T>
size_t constexpr print_array_to(Cursor output, T &array) {
  size_t N = array_size(array);
  size_t len = 0;
  
  len += output.write("[");

  for (size_t i = 0; i < N; i++) {
    len += print_value_to(output, array[i]);
    if (i < N - 1) {
      len += output.write(",");
    }
  }

  len += output.write("]");
  return len;
}

template <typename Cursor, typename T, size_t N>
size_t constexpr print_hex_to(Cursor output, T (&value)[N]) {
  size_t hex_size = sizeof(T) * 2;
  size_t len = 0;
  
  len += output.write("\"");
  
  for (size_t i = 0; i < N; i++) {
    len += output.printf("%0.*X", int(hex_size), value[i]);
  }
  
  len += output.write("\"");

  return len;
}

template <typename Cursor, typename... Args>
std::enable_if_t<key_value_checker_v<parsed_types, arguments_types, arguments_array_types, Args...>, size_t>
constexpr print_json(uint32_t mask, Cursor output, Args &&...args) {
  static_assert(sizeof...(Args) % 2 == 0,
                "Le nombre d'arguments doit être pair");

  size_t len = 0;
  
  int last_index = get_last_bitwise_mask_index(mask);

  len += output.write("{");

  len += print_key_value_pair(mask, 0, last_index, output, std::forward<Args>(args)...);

  len += output.write("}");

  return len;
}

constexpr int get_last_bitwise_mask_index(uint32_t mask) {
  if (mask == 0)
    return -1;

  size_t index = 0;
  while (mask >>= 1) {
    index++;
  }

  return index;
}
