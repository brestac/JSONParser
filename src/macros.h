#pragma once

#define NAMESPACE_JSON_BEGIN namespace JSON {
#define NAMESPACE_JSON_END }

#if !defined(DEBUG_PRINTLN) && !defined(DEBUG_PRINTF) && !defined(DEBUG_PRINT)
#if defined(DEBUG_ESP_PORT)
#define DEBUG_PRINTLN(x) DEBUG_ESP_PORT.println(x)
#define DEBUG_PRINTF(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#define DEBUG_PRINT(x) DEBUG_ESP_PORT.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#define DEBUG_PRINT(x)
#endif
#endif

#define TO_STRING(x) #x

#define CHECK_LOOP(MAX, STATEMENT)                                             \
  if (++GLOBAL_ITERATIONS > MAX_ITERATIONS) {                                  \
    JSON_DEBUG_ERROR("Too many iterations\n");                                 \
    STATEMENT                                                                  \
  }

#if defined(__GNUC__) && !defined(__clang__)
    #define COMPILER_NAME "GCC"
    #define COMPILER_MAJOR_VERSION __GNUC__
#elif defined(__clang__)
    #define COMPILER_NAME "Clang"
     #define COMPILER_MAJOR_VERSION __clang_major__
#endif

#define SUPPORTS_CONSTANT_EVALUATED COMPILER_MAJOR_VERSION >= 10
/*
Color   Code (foreground)
Black   \x1b[30m
Red     \x1b[31m
Green   \x1b[32m
Yellow  \x1b[33m
Blue    \x1b[34m
Magenta \x1b[35m
Cyan    \x1b[36m
White   \x1b[37m
Bright variants: add 1; before the code (e.g., \x1b[1;31m for bright red) or use
codes 90–97.
Background colors: replace 3 with 4 (e.g., \x1b[41m for red background) or use
100–107 for bright backgrounds. 256‑color mode: \x1b[38;5;<n>m (foreground) or
\x1b[48;5;<n>m (background), where <n> is 0‑255. True‑color (24‑bit) mode:
\x1b[38;2;<r>;<g>;<b>m (foreground) or \x1b[48;2;<r>;<g>;<b>m (background), with
RGB values 0‑255.
*/
#define COLOR_BLACK 30
#define COLOR_RED 31
#define COLOR_GREEN 32
#define COLOR_YELLOW 33
#define COLOR_BLUE 34
#define COLOR_MAGENTA 35
#define COLOR_CYAN 36
#define COLOR_WHITE 37
#define COLOR_BG_BLACK 40
#define COLOR_BG_RED 41
#define COLOR_BG_GREEN 42
#define COLOR_BG_BLUE 44
#define COLOR_BG_WHITE 47

#if defined(ARDUINO) && defined(JSON_DEBUG_MEM)
#define LOG_STACK(label)                                                       \
  uint32_t stack_remaining = ESP.getFreeContStack();                           \
  if (stack_remaining < GLOBAL_CONTEXT_STACK_SIZE) {                           \
    DEBUG_PRINTF(label " stack: %zu ", stack_remaining);                       \
    if constexpr (std::is_same_v<JSONCallbackObject, remove_cv_ref_t<V>>) {    \
      arg_value.key.print();                                                   \
    } else {                                                                   \
      DEBUG_PRINTLN();                                                         \
    }                                                                          \
    GLOBAL_CONTEXT_STACK_SIZE = stack_remaining;                               \
  }
#else
#define LOG_STACK(label)
#endif

#ifndef JSON_STRICT_MODE
#define SKIP_SPACES skip_spaces();
#else
#define SKIP_SPACES
#endif

#ifndef __GXX_RTTI
#define JSON_DEBUG_TYPES(format, ...)
#endif

#ifdef ARDUINO
#define PRINTF_COLOR(n, fmt, ...) DEBUG_PRINTF(fmt, ##__VA_ARGS__)
#else
#define PRINTF_COLOR(n, fmt, ...)                                              \
  DEBUG_PRINTF("\x1b[" TO_STRING(n) "m" fmt "\x1b[0m", ##__VA_ARGS__)
#endif

#if JSON_DEBUG_LEVEL > 0
#ifdef ARDUINO
#define JSON_DEBUG_COLOR(n, fmt, ...) DEBUG_PRINTF(fmt, ##__VA_ARGS__)
#else
#define JSON_DEBUG_COLOR(n, fmt, ...)                                          \
  DEBUG_PRINTF("\x1b[" TO_STRING(n) "m" fmt "\x1b[0m", ##__VA_ARGS__)
#endif
#else
#define JSON_DEBUG_COLOR(n, fmt, ...)
#endif

#if JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_INFO(format, ...)                                           \
  JSON_DEBUG_COLOR(COLOR_BLACK, format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_INFO(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_WARNING(format, ...)                                        \
  JSON_DEBUG_COLOR(COLOR_YELLOW, format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_WARNING(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 3 || JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_ERROR(format, ...)                                          \
  JSON_DEBUG_COLOR(COLOR_RED, format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_ERROR(format, ...)
#endif
 
#define FROM_JSON_OVERRIDE(...)                                                \
  MaskType updated = 0;                                                        \
  template <typename T> JSON::ParseResult fromJSON(T&& input) {                \
    using _SelfT = remove_cv_ref_t<decltype(*this)>;                           \
    return JSON::_parse_impl<true, _SelfT>(                                    \
        updated, input, CREATE_DISPATCH_TABLE(__VA_ARGS__));                   \
  }                                                                            \
  template <typename T> JSON::ParseResult fromJSON(T* input) {                 \
    using _SelfT = remove_cv_ref_t<decltype(*this)>;                           \
    return JSON::_parse_impl<true, _SelfT>(                                    \
        updated, input, CREATE_DISPATCH_TABLE(__VA_ARGS__));                   \
  }                                                                            \
  JSON::ParseResult fromJSON(const char* input, size_t size) {                 \
    using _SelfT = remove_cv_ref_t<decltype(*this)>;                           \
    return JSON::_parse_impl<true, _SelfT>(                                    \
        updated, input, size, CREATE_DISPATCH_TABLE(__VA_ARGS__));             \
  }
#define TO_JSON_OVERRIDE(...)                                                  \
  template <typename T> size_t toJSON(T& output, bool updates = false) {       \
    uint32_t mask = updates ? this->updated : 0;                               \
    return JSON::print(mask, output, KV_LIST(__VA_ARGS__));                    \
  }                                                                            \
  size_t toJSON(PointerCursorWriter& output, bool updates = false) override {  \
    uint32_t mask = updates ? this->updated : 0;                               \
    return JSON::_print(mask, output, KV_LIST(__VA_ARGS__));                   \
  }                                                                            \
  size_t toJSON(StreamCursorWriter& output, bool updates = false) override {   \
    uint32_t mask = updates ? this->updated : 0;                               \
    return JSON::_print(mask, output, KV_LIST(__VA_ARGS__));                   \
  }

#define CREATE_DISPATCH_TABLE(...) create_dispatch_tuple(KV_LIST(__VA_ARGS__))

#define JSON_DECODER_IMPL(...) FROM_JSON_OVERRIDE(__VA_ARGS__)

#define JSON_ENCODER_IMPL(...)                                                 \
  using JSONObject::toJSON;                                                    \
  TO_JSON_OVERRIDE(__VA_ARGS__)

#define JSON_SERIALIZE_IMPL(...)                                               \
  using JSONObject::toJSON;                                                    \
  FROM_JSON_OVERRIDE(__VA_ARGS__)                                              \
  TO_JSON_OVERRIDE(__VA_ARGS__)

// Macro pour créer les paires
#define KV_PAIR(x) #x, x

#define KV_LIST_1(a) KV_PAIR(a)
#define KV_LIST_2(a, b) KV_PAIR(a), KV_PAIR(b)
#define KV_LIST_3(a, b, c) KV_PAIR(a), KV_PAIR(b), KV_PAIR(c)
#define KV_LIST_4(a, b, c, d) KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d)
#define KV_LIST_5(a, b, c, d, e)                                               \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e)
#define KV_LIST_6(a, b, c, d, e, f)                                            \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f)
#define KV_LIST_7(a, b, c, d, e, f, g)                                         \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g)
#define KV_LIST_8(a, b, c, d, e, f, g, h)                                      \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h)
#define KV_LIST_9(a, b, c, d, e, f, g, h, i)                                   \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i)
#define KV_LIST_10(a, b, c, d, e, f, g, h, i, j)                               \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j)
#define KV_LIST_11(a, b, c, d, e, f, g, h, i, j, k)                            \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k)
#define KV_LIST_12(a, b, c, d, e, f, g, h, i, j, k, l)                         \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l)
#define KV_LIST_13(a, b, c, d, e, f, g, h, i, j, k, l, m)                      \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m)
#define KV_LIST_14(a, b, c, d, e, f, g, h, i, j, k, l, m, n)                   \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n)
#define KV_LIST_15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)                \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o)
#define KV_LIST_16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)             \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p)
#define KV_LIST_17(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q)          \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q)
#define KV_LIST_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r)       \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r)
#define KV_LIST_19(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s)    \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s)
#define KV_LIST_20(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t) \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t)
#define KV_LIST_21(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u)                                                          \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u)
#define KV_LIST_22(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v)                                                       \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v)
#define KV_LIST_23(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w)                                                    \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w)
#define KV_LIST_24(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x)                                                 \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x)
#define KV_LIST_25(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y)                                              \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y)
#define KV_LIST_26(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z)                                           \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z)
#define KV_LIST_27(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z, a2)                                       \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z), KV_PAIR(a2)
#define KV_LIST_28(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z, a2, b2)                                   \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z), KV_PAIR(a2), KV_PAIR(b2)
#define KV_LIST_29(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z, a2, b2, c2)                               \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z), KV_PAIR(a2), KV_PAIR(b2), KV_PAIR(c2)
#define KV_LIST_30(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z, a2, b2, c2, d2)                           \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z), KV_PAIR(a2), KV_PAIR(b2), KV_PAIR(c2),           \
      KV_PAIR(d2)
#define KV_LIST_31(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z, a2, b2, c2, d2, e2)                       \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z), KV_PAIR(a2), KV_PAIR(b2), KV_PAIR(c2),           \
      KV_PAIR(d2), KV_PAIR(e2)
#define KV_LIST_32(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, \
                   u, v, w, x, y, z, a2, b2, c2, d2, e2, f2)                   \
  KV_PAIR(a), KV_PAIR(b), KV_PAIR(c), KV_PAIR(d), KV_PAIR(e), KV_PAIR(f),      \
      KV_PAIR(g), KV_PAIR(h), KV_PAIR(i), KV_PAIR(j), KV_PAIR(k), KV_PAIR(l),  \
      KV_PAIR(m), KV_PAIR(n), KV_PAIR(o), KV_PAIR(p), KV_PAIR(q), KV_PAIR(r),  \
      KV_PAIR(s), KV_PAIR(t), KV_PAIR(u), KV_PAIR(v), KV_PAIR(w), KV_PAIR(x),  \
      KV_PAIR(y), KV_PAIR(z), KV_PAIR(a2), KV_PAIR(b2), KV_PAIR(c2),           \
      KV_PAIR(d2), KV_PAIR(e2), KV_PAIR(f2)

#define GET_KV_LIST(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13,    \
                    _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24,     \
                    _25, _26, _27, _28, _29, _30, _31, _32, NAME, ...)         \
  NAME

#define KV_LIST(...)                                                           \
  GET_KV_LIST(__VA_ARGS__, KV_LIST_32, KV_LIST_31, KV_LIST_30, KV_LIST_29,     \
              KV_LIST_28, KV_LIST_27, KV_LIST_26, KV_LIST_25, KV_LIST_24,      \
              KV_LIST_23, KV_LIST_22, KV_LIST_21, KV_LIST_20, KV_LIST_19,      \
              KV_LIST_18, KV_LIST_17, KV_LIST_16, KV_LIST_15, KV_LIST_14,      \
              KV_LIST_13, KV_LIST_12, KV_LIST_11, KV_LIST_10, KV_LIST_9,       \
              KV_LIST_8, KV_LIST_7, KV_LIST_6, KV_LIST_5, KV_LIST_4,           \
              KV_LIST_3, KV_LIST_2, KV_LIST_1)                                 \
  (__VA_ARGS__)
