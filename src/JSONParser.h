#pragma once

#include "JSONStreamParser.h"
#include "StreamCursor.h"
#include "utils.h"
#include <string_view>

NAMESPACE_JSON_BEGIN

template <typename... Args>
using enable_if_args_valid =
    std::enable_if_t<key_value_checker_v<parsed_types, arguments_types,
                                         arguments_array_types, Args...>,
                     ParseResult>;

template <typename T>
using enable_if_cursor =
    std::enable_if_t<is_cursor_reader_v<remove_cvref_t<T>>, ParseResult>;

template <typename T>
using enable_if_pointer_reader_compatible =
    std::enable_if_t<is_cursor_reader_v<T>, ParseResult>;

template <typename T>
using enable_if_stream_compatible =
    std::enable_if_t<std::is_base_of<Stream, remove_cvref_t<T>>::value,
                     ParseResult>;

template <typename T>
using enable_if_json_data_container_compatible =
    std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>;

////////////////////////////////////////////////////////////
//  Parse With Cursor
///////////////////////////////////////////////////////////
template <typename Cursor, typename... Args>
enable_if_args_valid<Args...> _parse(uint32_t& mask, Cursor &cursor, Args &&...args) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);

  bool automask = are_generic_keys(std::forward<Args>(args)...);
  parser.setAutomask(automask);

  parser.parse(std::forward<Args>(args)...);

  mask = parser.keyMask();

  uint64_t end = now();

  return ParseResult(&parser, end - start);
}

template <typename Cursor, typename... Args>
enable_if_args_valid<Args...> __parse(Cursor &cursor, Args &&...args) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser.setUseMask(false);
  parser.parse(std::forward<Args>(args)...);

  uint64_t end = now();

  return ParseResult(&parser, end - start);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, const PointerCursorReader &cursor,
                  Args &&...args) {
  // const PointerCursorReader c = cursor;
  return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, const char *input, Args &&...args) {
  const PointerCursorReader cursor(input, str_length(input));
  return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <size_t N, typename... Args>
ParseResult parse(uint32_t &mask, const char (&input)[N], Args &&...args) {
  const PointerCursorReader cursor(input, N - 1);
  return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, StreamCursor &stream, Args &&...args) {
  return _parse(mask, stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
enable_if_stream_compatible<T> parse(uint32_t &mask, T &input, Args &&...args) {
  StreamCursor stream(input);
  return _parse(mask, stream, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  Parse With Callback
///////////////////////////////////////////////////////////

ParseResult parse(StreamCursor &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT");
  return __parse(cursor, cb_obj);
}

ParseResult parse(const PointerCursorReader &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT");
  return __parse(cursor, cb_obj);
}

////////////////////////////////////////////////////////////
//  Parse Top level array
///////////////////////////////////////////////////////////
template <typename T>
enable_if_json_data_container_compatible<T>
parse(uint32_t &mask, const PointerCursorReader &cursor, T &jsonObjects) {
  // const PointerCursorReader c = cursor;
  return _parse(mask, cursor, jsonObjects);
}

// template <typename T>
// std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>
// parse(uint32_t &mask, StreamCursor& cursor, T &jsonObjects) {
//   return _parse(mask, cursor, jsonObjects);
// }

NAMESPACE_JSON_END

JSON::ParseResult UnknownValueType::fromJSON(JSON::StreamCursor &cursor) {
  return JSON::__parse(cursor);
}

JSON::ParseResult
UnknownValueType::fromJSON(const JSON::PointerCursorReader &cursor) {
  // const PointerCursorReader c = cursor;
  return JSON::__parse(cursor);
}

JSON::ParseResult
JSONCallbackObject::fromJSON(const JSON::PointerCursorReader &cursor) {
  // const PointerCursorReader c = cursor;
  return JSON::__parse(cursor, *this);
}

JSON::ParseResult JSONCallbackObject::fromJSON(JSON::StreamCursor &cursor) {
  return JSON::__parse(cursor, *this);
}

// #endif
