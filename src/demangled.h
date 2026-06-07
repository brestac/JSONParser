#pragma once

#include <cstdio>
#include <cstdlib>

#ifdef __GXX_RTTI
#include <cxxabi.h>
#include <memory>
#include <typeinfo>
#endif

#include "macros.h"

template <typename T, typename... Args>
void print_demangled_type(const char *format, T &value, Args &&...args);

template <typename... Args>
void print_demangled_types(const char *format, Args &&...args);

// #ifdef __GXX_RTTI
// char *demanglerconst char* name);
// template <typename Tuple, size_t... Is>
// void printf_impl(const char *format, Tuple &t, std::index_sequence<Is...>);
// #endif

#ifdef __GXX_RTTI
// Wrapper RAII minimal
struct DemangledName {
  const char *str;
  bool owned;

  explicit DemangledName(const char* type_name) {
    int status;
    char *d = abi::__cxa_demangle(type_name, nullptr, nullptr, &status);
    if (status == 0) {
      str = d;
      owned = true;
    } else {
      str = type_name;
      owned = false;
    }
  }

  ~DemangledName() {
    if (owned)
      std::free(const_cast<char *>(str));
    // printf("DemangledName destructor\n");
  }

  DemangledName(const DemangledName &) = delete;
  DemangledName &operator=(const DemangledName &) = delete;
  DemangledName(DemangledName &&other) : str(other.str), owned(other.owned) {
    other.owned = false; // Transferer la propriete
  }

  operator const char *() const { return str; }
};
template <typename Tuple, size_t... Is>
void printf_impl(const char *format, Tuple &t, std::index_sequence<Is...>) {
  DEBUG_PRINTF(format, static_cast<const char *>(std::get<Is>(t))...);
}
#endif

template <typename... Args>
void print_demangled_types(const char *format, Args &&...args) {
#ifdef __GXX_RTTI
  // Tous les DemangledName sont crees et vivent jusqu'a la fin de la fonction
  auto names = std::make_tuple(DemangledName(typeid(args).name())...);
  DEBUG_PRINTF("\x1b[31m");
  printf_impl(format, names, std::index_sequence_for<Args...>{});
  DEBUG_PRINTF("\x1b[0m");
#endif
}

template <typename T, typename... Args>
void print_demangled_type(const char *format, T &value, Args &&...args) {
#ifdef __GXX_RTTI
  DemangledName demangled(typeid(value).name());
  DEBUG_PRINTF("\x1b[31m");
  printf_impl(format, static_cast<const char *>(demangled), std::forward<Args>(args)...);
  DEBUG_PRINTF("\x1b[0m");
#endif
}

template <typename T> void print_demangled_type(T &value) {
  print_demangled_type("%s\n", value);
}

#if JSON_DEBUG_LEVEL > 0
#ifdef ARDUINO
#define JSON_DEBUG_TYPES(format, ...)
#else
#define JSON_DEBUG_TYPES(format, ...)                                          \
  print_demangled_types("\x1b[35m" format "\x1b[0m", ##__VA_ARGS__);
#endif
#else
#define JSON_DEBUG_TYPES(format, ...)
#endif
