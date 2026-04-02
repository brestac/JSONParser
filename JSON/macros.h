#pragma once

#ifndef DEV_MODE
#define DEV_MODE 0
#endif

#define NAMESPACE_JSON_BEGIN namespace JSON {
#define NAMESPACE_JSON_END }

#if JSON_DEBUG_LEVEL > 0 && !defined(JSON_DEBUG_PRINTF) && !defined(ARDUINO)
#define JSON_DEBUG_PRINTF ::printf
#else
#define JSON_DEBUG_PRINTF(...)
#endif

#define COLOR_0 "\x1b[30m"
#define COLOR_1 "\x1b[32m"
#define COLOR_2 "\x1b[33m"
#define COLOR_3 "\x1b[31m"
#define COLOR_END "\x1b[0m"

#define CONCAT(a, b) CONCAT_HELPER(a, b)
#define CONCAT_HELPER(a, b) a##b

#define JSON_DEBUG_COLOR CONCAT(COLOR_, JSON_DEBUG_LEVEL)

#if JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_INFO(format, ...) JSON_DEBUG_PRINTF(format, ##__VA_ARGS__)
#else
#define JSON_DEBUG_INFO(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_WARNING(format, ...)                                        \
  JSON_DEBUG_PRINTF(JSON_DEBUG_COLOR format COLOR_END, ##__VA_ARGS__)
#else
#define JSON_DEBUG_WARNING(format, ...)
#endif

#if JSON_DEBUG_LEVEL == 3 || JSON_DEBUG_LEVEL == 2 || JSON_DEBUG_LEVEL == 1
#define JSON_DEBUG_ERROR(format, ...)                                          \
  JSON_DEBUG_PRINTF(JSON_DEBUG_COLOR format COLOR_END, ##__VA_ARGS__)
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

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, NAME,     \
                  ...)                                                         \
  NAME

#define MACRO(...)                                                             \
  GET_MACRO(__VA_ARGS__, MACRO_12, MACRO_11, MACRO_10, MACRO_9, MACRO_8,       \
            MACRO_7, MACRO_6, MACRO_5, MACRO_4, MACRO_3, MACRO_2, MACRO_1)     \
  (__VA_ARGS__)

#define TO_JSON(a, b, ...) JSON::print(a, b, MACRO(__VA_ARGS__))
#define FROM_JSON(a, b, ...) JSON::parse(a, b, MACRO(__VA_ARGS__))

#ifdef ARDUINO
#define STREAM_CURSOR_OVERRIDE(...)                                            \
  JSON::ParseResult fromJSON(const PointerCursorReader &cursor) override {     \
    const PointerCursorReader _c = cursor;                                     \
    return JSON::_parse(this->updated, _c, MACRO(__VA_ARGS__));                \
  }                                                                            \
  size_t toJSON(PointerCursorWriter &writer, bool updates = true) override {   \
    // PointerCursorWriter _c = writer;                                     \
    size_t mask = updates ? this->updated : 0;                                 \
    return JSON::print(mask, writer, MACRO(__VA_ARGS__));                      \
  }                                                                            \
  template <typename T>                                                        \
  std::enable_if_t<std::is_base_of_v<Stream, T>, JSON::ParseResult> fromJSON(  \
      T &cursor) {                                                             \
    StreamCursor streamCursor(cursor);                                         \
    return fromJSON(streamCursor);                                             \
  }                                                                            \
  JSON::ParseResult fromJSON(StreamCursor &cursor) override {                  \
    return JSON::parse(this->updated, cursor, MACRO(__VA_ARGS__));             \
  }                                                                            \
  template <typename T>                                                        \
  std::enable_if_t<std::is_base_of_v<Stream, T>, size_t> toJSON(               \
      T &cursor, bool updates = true) {                                        \
    StreamCursor streamCursor(cursor);                                         \
    return toJSON(streamCursor, updates);                                      \
  }                                                                            \
  size_t toJSON(StreamCursor &cursor, bool updates = true) override {          \
    size_t mask = updates ? this->updated : 0;                                 \
    return JSON::print(mask, cursor, MACRO(__VA_ARGS__));                      \
  }
#else
#define STREAM_CURSOR_OVERRIDE(...)                                            \
  JSON::ParseResult fromJSON(const PointerCursorReader &cursor) override {     \
    return JSON::parse(this->updated, cursor, MACRO(__VA_ARGS__));             \
  }                                                                            \
  size_t toJSON(PointerCursorWriter &writer, bool updates = true) override {   \
    size_t mask = updates ? this->updated : 0;                                 \
    return JSON::print(mask, writer, MACRO(__VA_ARGS__));                      \
  }                                                                            \
  size_t toJSON(PointerCursorPrinter &writer, bool updates = true) override {  \
    size_t mask = updates ? this->updated : 0;                                 \
    return JSON::print(mask, writer, MACRO(__VA_ARGS__));                      \
  }
#endif

#define TO_JSON_FROM_JSON(...)                                                 \
  using JSONObject::fromJSON;                                                  \
  using JSONObject::toJSON;                                                    \
  STREAM_CURSOR_OVERRIDE(__VA_ARGS__)
