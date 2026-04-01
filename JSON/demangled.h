#pragma once

#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>
#include <typeinfo>
#include <memory>

#ifdef ARDUINO
  #include <Stream.h>
  #define PRINT_FUNC Serial.printf
#else
  #define PRINT_FUNC ::printf
#endif

template <typename T, typename... Args>
void print_demangled_type(const char *format, T &value, Args &&...args);

template <typename... Args>
void print_demangled_types(const char* format, Args&&... args);

#ifdef __GXX_RTTI
char *demangler(const char *name);
template <typename Tuple, size_t... Is>
void printf_impl(const char* format, Tuple& t, std::index_sequence<Is...>);
#endif


#ifdef __GXX_RTTI
// Wrapper RAII minimal
struct DemangledName {
    const char* str;
    bool owned;

    explicit DemangledName(const char* name) {
        int status;
        char* d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
        if (status == 0) { str = d; owned = true; }
        else             { str = name; owned = false; }
    }

    ~DemangledName() { 
        if (owned) std::free(const_cast<char*>(str));
        //printf("DemangledName destructor\n");
    }

    DemangledName(const DemangledName&)            = delete;
    DemangledName& operator=(const DemangledName&) = delete;
    DemangledName(DemangledName&& other) 
        : str(other.str), owned(other.owned) {
        other.owned = false;  // Transferer la propriete
    }

    operator const char*() const { return str; }
};
template <typename Tuple, size_t... Is>
void printf_impl(const char* format, Tuple& t, std::index_sequence<Is...>) {
  PRINT_FUNC(format, static_cast<const char*>(std::get<Is>(t))...);
}
#endif

template <typename... Args>
void print_demangled_types(const char* format, Args&&... args) {
#ifdef __GXX_RTTI
    // Tous les DemangledName sont crees et vivent jusqu'a la fin de la fonction
    auto names = std::make_tuple(DemangledName(typeid(args).name())...);
    PRINT_FUNC("\x1b[31m");
    printf_impl(format, names, std::index_sequence_for<Args...>{});
    PRINT_FUNC("\x1b[0m");
#else
  JSON_DEBUG_WARNING("RTTI not enabled");
#endif
}

template <typename T, typename... Args>
void print_demangled_type(const char *format, T &value, Args &&...args) {
#ifdef __GXX_RTTI
  DemangledName demangled(typeid(value).name());
  PRINT_FUNC("\x1b[31m");
  printf(format, static_cast<const char*>(demangled), std::forward<Args>(args)...);
  PRINT_FUNC("\x1b[0m");
#else
  JSON_DEBUG_WARNING("RTTI not enabled");
#endif
}

template <typename T>
void print_demangled_type(T &value) {
  print_demangled_type("%s\n", value);
}

#define COLOR_0 "\x1b[30m"
#define COLOR_1 "\x1b[32m"
#define COLOR_2 "\x1b[33m"
#define COLOR_3 "\x1b[31m"
#define COLOR_END "\x1b[0m"

#if JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_TYPES(format, ...) print_demangled_types(JSON_DEBUG_COLOR format COLOR_END, ##__VA_ARGS__);
#elif JSON_DEBUG_LEVEL == 2
#define JSON_DEBUG_TYPES(format, ...) print_demangled_types(JSON_DEBUG_COLOR format COLOR_END, ##__VA_ARGS__);
#elif JSON_DEBUG_LEVEL == 3
#define JSON_DEBUG_TYPES(format, ...) print_demangled_types(JSON_DEBUG_COLOR format COLOR_END, ##__VA_ARGS__);
#else
#define JSON_DEBUG_TYPES(format, ...)
#endif