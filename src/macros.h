#pragma once
/*
Color	Code (foreground)
Black	\x1b[30m
Red	\x1b[31m
Green	\x1b[32m
Yellow	\x1b[33m
Blue	\x1b[34m
Magenta	\x1b[35m
Cyan	\x1b[36m
White	\x1b[37m
Bright variants: add 1; before the code (e.g., \x1b[1;31m for bright red) or use
codes 90–97. Background colors: replace 3 with 4 (e.g., \x1b[41m for red
background) or use 100–107 for bright backgrounds. 256‑color mode:
\x1b[38;5;<n>m (foreground) or \x1b[48;5;<n>m (background), where <n> is 0‑255.
True‑color (24‑bit) mode: \x1b[38;2;<r>;<g>;<b>m (foreground) or
\x1b[48;2;<r>;<g>;<b>m (background), with RGB values 0‑255.
*/
#ifndef DEV_MODE
#define DEV_MODE 0
#endif

#define PRINTF_COLOR(n, s...)                                                  \
  DEBUG_PRINTF("\x1b[" #n "m");                                                \
  DEBUG_PRINTF(s);                                                             \
  DEBUG_PRINTF("\x1b[0m")
#define PRINTF_BLACK(s...) PRINTF_COLOR(30, s)
#define PRINTF_RED(s...) PRINTF_COLOR(31, s)
#define PRINTF_GREEN(s...) PRINTF_COLOR(32, s)
#define PRINTF_YELLOW(s...) PRINTF_COLOR(33, s)
#define PRINTF_BLUE(s...) PRINTF_COLOR(34, s)
#define PRINTF_MAGENTA(s...) PRINTF_COLOR(35, s)
#define PRINTF_CYAN(s...) PRINTF_COLOR(36, s)
#define PRINTF_WHITE(s...) PRINTF_COLOR(37, s)

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

// #define CONCAT(a, b) CONCAT_HELPER(a, b)
// #define CONCAT_HELPER(a, b) a##b

#if JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_INFO(format, ...) PRINTF_BLACK(format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_INFO(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_WARNING(format, ...) PRINTF_YELLOW(format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_WARNING(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 3 || JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_ERROR(format, ...) PRINTF_RED(format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_ERROR(format, ...)
#endif

// #ifndef __GXX_RTTI
//   JSON_DEBUG_WARNING("RTTI not enabled\n");
// #endif

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

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, NAME,     \
                  ...)                                                         \
  NAME

#define MACRO(...)                                                             \
  GET_MACRO(__VA_ARGS__, MACRO_12, MACRO_11, MACRO_10, MACRO_9, MACRO_8,       \
            MACRO_7, MACRO_6, MACRO_5, MACRO_4, MACRO_3, MACRO_2, MACRO_1)     \
  (__VA_ARGS__)

#define FROM_JSON_OVERRIDE(...)                                                \
  template <typename T> JSON::ParseResult fromJSON(T &input) {                 \
    return JSON::parse(this->updated, input, MACRO(__VA_ARGS__));              \
  }                                                                            \
  JSON::ParseResult fromJSON(const PointerCursorReader &cursor) override {     \
    return JSON::_parse(this->updated, cursor, MACRO(__VA_ARGS__));            \
  }                                                                            \
  JSON::ParseResult fromJSON(StreamCursor &cursor) override {                  \
    return JSON::parse(this->updated, cursor, MACRO(__VA_ARGS__));             \
  }
/*
template <typename T>                                                        \
std::enable_if_t<std::is_base_of_v<Stream, T>, JSON::ParseResult> fromJSON(  \
    T &stream) {                                                             \
  StreamCursor streamCursor(stream);                                         \
  return fromJSON(streamCursor);                                             \
}                                                                            \

*/

#define TO_JSON_OVERRIDE(...)                                                  \
  template <typename T> size_t toJSON(T &output, bool updates = true) {        \
    size_t mask = updates ? this->updated : 0;                                 \
    return JSON::print(mask, output, MACRO(__VA_ARGS__));                      \
  }

#define JSON_DECODER_IMPL(...)                                                 \
  using JSONObject::fromJSON;                                                  \
  FROM_JSON_OVERRIDE(__VA_ARGS__)

#define JSON_ENCODER_IMPL(...)                                                 \
  using JSONObject::toJSON;                                                    \
  TO_JSON_OVERRIDE(__VA_ARGS__)

#define JSON_SERIALIZE_IMPL(...)                                               \
  using JSONObject::fromJSON;                                                  \
  using JSONObject::toJSON;                                                    \
  FROM_JSON_OVERRIDE(__VA_ARGS__)                                              \
  TO_JSON_OVERRIDE(__VA_ARGS__)
