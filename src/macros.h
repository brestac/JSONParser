#pragma once
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

#define NAMESPACE_JSON_BEGIN namespace JSON {
#define NAMESPACE_JSON_END }

#if !defined(DEBUG_PRINTLN) && !defined(DEBUG_PRINTF) && !defined(DEBUG_PRINT)
#if defined(DEBUG_ESP_PORT)
#define DEBUG_PRINTLN(x) DEBUG_ESP_PORT.println(x)
#define DEBUG_PRINTF(x...) DEBUG_ESP_PORT.printf(x)
#define DEBUG_PRINT(x) DEBUG_ESP_PORT.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(x...)
#define DEBUG_PRINT(x)
#endif
#endif

#define TO_STRING(x) #x

#define CHECK_LOOP(MAX,RETURN_VALUE)                                           \
  static size_t iteration = 0;                                                 \
  if (iteration % YIELD_EVERY == 0) yield();                                   \
  if (++iteration > MAX) {                                                     \
    JSON_DEBUG_ERROR("Too many iterations\n");                                 \
    return RETURN_VALUE;                                                       \
  }

#define CHECK_LOOP_VOID(MAX,STATEMENT)                                         \
static size_t iteration = 0;                                                   \
if (iteration % YIELD_EVERY == 0) yield();                                     \
if (++iteration > MAX) {                                                       \
  JSON_DEBUG_ERROR("Too many iterations\n");                                   \
  STATEMENT                                                                    \
  return;                                                                      \
}

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
#define LOG_STACK(label) \
  uint32_t stack_remaining = ESP.getFreeContStack();\
  if (stack_remaining < GLOBAL_CONTEXT_STACK_SIZE) {\
    DEBUG_PRINTF(label" stack: %zu ", stack_remaining);\
    if constexpr (std::is_same_v<JSONCallbackObject, remove_cv_ref_t<V>>) {\
      arg_value.key.print();\
    } else {\
      DEBUG_PRINTLN();\
    }\
    GLOBAL_CONTEXT_STACK_SIZE = stack_remaining;\
  }
#else
#define LOG_STACK(label)
#endif

#ifndef JSON_STRICT_MODE
  #define SKIP_SPACES() skip_spaces()
#else
  #define SKIP_SPACES()
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

// Macro pour créer les paires
#define PAIR(x) #x, x
#define PAIR_IDX(idx, x) #x "[" #idx "]", x

#define MACRO_1(a) PAIR(a)
#define MACRO_2(a, b) PAIR(a), PAIR(b)
#define MACRO_3(a, b, c) PAIR(a), PAIR(b), PAIR(c)
#define MACRO_4(a, b, c, d) PAIR(a), PAIR(b), PAIR(c), PAIR(d)
#define MACRO_5(a, b, c, d, e) PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e)
#define MACRO_6(a, b, c, d, e, f)                                              \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f)
#define MACRO_7(a, b, c, d, e, f, g)                                           \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g)
#define MACRO_8(a, b, c, d, e, f, g, h)                                        \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h)
#define MACRO_9(a, b, c, d, e, f, g, h, i)                                     \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i)
#define MACRO_10(a, b, c, d, e, f, g, h, i, j)                                 \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j)
#define MACRO_11(a, b, c, d, e, f, g, h, i, j, k)                              \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k)
#define MACRO_12(a, b, c, d, e, f, g, h, i, j, k, l)                           \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l)
#define MACRO_13(a, b, c, d, e, f, g, h, i, j, k, l, m)                        \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m)
#define MACRO_14(a, b, c, d, e, f, g, h, i, j, k, l, m, n)                     \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n)
#define MACRO_15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)                  \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o)
#define MACRO_16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)               \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p)
#define MACRO_17(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q)            \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q)
#define MACRO_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r)         \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r)
#define MACRO_19(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s)      \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s)
#define MACRO_20(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t)   \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t)
#define MACRO_21(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u)                                                            \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u)
#define MACRO_22(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v)                                                         \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v)
#define MACRO_23(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w)                                                      \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w)
#define MACRO_24(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x)                                                   \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x)
#define MACRO_25(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y)                                                \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y)
#define MACRO_26(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z)                                             \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z)
#define MACRO_27(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z, a2)                                         \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z), PAIR(a2)
#define MACRO_28(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z, a2, b2)                                     \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z), PAIR(a2), PAIR(b2)
#define MACRO_29(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z, a2, b2, c2)                                 \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z), PAIR(a2), PAIR(b2), PAIR(c2)
#define MACRO_30(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z, a2, b2, c2, d2)                             \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z), PAIR(a2), PAIR(b2), PAIR(c2), PAIR(d2)
#define MACRO_31(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z, a2, b2, c2, d2, e2)                         \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z), PAIR(a2), PAIR(b2), PAIR(c2), PAIR(d2), PAIR(e2)
#define MACRO_32(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t,   \
                 u, v, w, x, y, z, a2, b2, c2, d2, e2, f2)                     \
  PAIR(a), PAIR(b), PAIR(c), PAIR(d), PAIR(e), PAIR(f), PAIR(g), PAIR(h),      \
      PAIR(i), PAIR(j), PAIR(k), PAIR(l), PAIR(m), PAIR(n), PAIR(o), PAIR(p),  \
      PAIR(q), PAIR(r), PAIR(s), PAIR(t), PAIR(u), PAIR(v), PAIR(w), PAIR(x),  \
      PAIR(y), PAIR(z), PAIR(a2), PAIR(b2), PAIR(c2), PAIR(d2), PAIR(e2),      \
      PAIR(f2)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, \
                  _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,  \
                  _27, _28, _29, _30, _31, _32, NAME, ...)                     \
  NAME

#define MACRO(...)                                                             \
  GET_MACRO(__VA_ARGS__, MACRO_32, MACRO_31, MACRO_30, MACRO_29, MACRO_28,     \
            MACRO_27, MACRO_26, MACRO_25, MACRO_24, MACRO_23, MACRO_22,        \
            MACRO_21, MACRO_20, MACRO_19, MACRO_18, MACRO_17, MACRO_16,        \
            MACRO_15, MACRO_14, MACRO_13, MACRO_12, MACRO_11, MACRO_10,        \
            MACRO_9, MACRO_8, MACRO_7, MACRO_6, MACRO_5, MACRO_4, MACRO_3,     \
            MACRO_2, MACRO_1)                                                  \
  (__VA_ARGS__)

// APRÈS
#define FROM_JSON_OVERRIDE(...)                                                \
  template <typename T>\
  JSON::ParseResult fromJSON(T &&input) {     \
    using _SelfT = remove_cv_ref_t<decltype(*this)>;                            \
    return JSON::_parse_impl<true, _SelfT>(         \
        this->updated, input, MACRO(__VA_ARGS__));                             \
  }
#define TO_JSON_OVERRIDE(...)                                                  \
  template <typename T> size_t toJSON(T &output, bool updates = false) {       \
    uint32_t mask = updates ? this->updated : 0;                               \
    return JSON::print(mask, output, MACRO(__VA_ARGS__));                      \
  }                                                                            \
  size_t toJSON(PointerCursorWriter &output, bool updates = false) override {  \
    uint32_t mask = updates ? this->updated : 0;                               \
    return JSON::_print(mask, output, MACRO(__VA_ARGS__));                     \
  }                                                                            \
  size_t toJSON(StreamCursorWriter &output, bool updates = false) override {         \
    uint32_t mask = updates ? this->updated : 0;                               \
    return JSON::_print(mask, output, MACRO(__VA_ARGS__));                     \
  }

#define JSON_DECODER_IMPL(...)                                                 \
  FROM_JSON_OVERRIDE(__VA_ARGS__)

#define JSON_ENCODER_IMPL(...)                                                 \
  using JSONObject::toJSON;                                                    \
  TO_JSON_OVERRIDE(__VA_ARGS__)

#define JSON_SERIALIZE_IMPL(...)                                               \
  using JSONObject::toJSON;                                                    \
  FROM_JSON_OVERRIDE(__VA_ARGS__)                                              \
  TO_JSON_OVERRIDE(__VA_ARGS__)
