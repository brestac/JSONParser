#include "JSONStreamParser.h"
#include "macros.h"

NAMESPACE_JSON_BEGIN
//  using enable_parse = std::enable_if_t<key_value_checker_v<parsed_types,
//  arguments_array_types, arguments_array_array_types, Args...>, int32_t >;
  
template <typename Cursor>
std::enable_if_t<is_cursor_v<Cursor>, JSON::ParseResult>
_parse(Cursor& cursor, const JSONCallback &cb, int arrayIndex = -1);

JSON::ParseResult parse(PointerCursor cursor, const JSONCallback &cb, int arrayIndex = -1);
  
template <typename Cursor, typename... Args>
std::enable_if_t<
  is_cursor_v<Cursor> && key_value_checker_v<parsed_types, arguments_types,
                        arguments_array_types /*,arguments_array_array_types*/,
                        Args...>,
    ParseResult>
_parse(uint32_t &mask, const Cursor& cursor, Args &&...args);

template <typename... Args>
JSON::ParseResult parse(uint32_t &mask, PointerCursor cursor, Args &&...args);
  
template <typename Cursor, typename T>
std::enable_if_t<is_cursor_v<Cursor> && is_derived_json_data_container_v<T>, ParseResult>
_parse(uint32_t &mask, Cursor& cursor, T &jsonObjects);

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, JSON::ParseResult>
parse(uint32_t &mask, PointerCursor cursor, T &jsonObjects);
  
template <typename Cursor>
ParseResult parseResult(JSONParserBase<Cursor> &parser, uint64_t duration) {
  return ParseResult(parser.parsed_length(), parser.nKeys, parser.nParsed,
                     parser.nConverted, parser.nUpdated, parser.error(),
                     duration,
                     parser._state == JSONParserBase<Cursor>::ParserState::STOPPED);
}

////////////////////////////////////////////////////////////
//  Parse With Callback
///////////////////////////////////////////////////////////
template <typename Cursor>
std::enable_if_t<is_cursor_v<Cursor>, JSON::ParseResult>
_parse(Cursor& cursor, const JSONCallback &cb, int arrayIndex) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser.setArrayIndex(arrayIndex);
  parser.parse(cb);

  uint64_t end = now();

  return parseResult(parser, end - start);
}

JSON::ParseResult parse(PointerCursor cursor, const JSONCallback& cb, int arrayIndex) {
  return _parse(cursor, cb, arrayIndex);
}
  
////////////////////////////////////////////////////////////
//  Parse With Cursor
///////////////////////////////////////////////////////////
template <typename Cursor, typename... Args>
std::enable_if_t<is_cursor_v<Cursor> && key_value_checker_v<parsed_types, arguments_types,
                                     arguments_array_types, Args...>,
                 JSON::ParseResult>
_parse(uint32_t &mask, const Cursor& cursor, Args &&...args) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser._automask = are_keys(std::forward<Args>(args)...);
  parser.parse(std::forward<Args>(args)...);
  mask = parser.keyMask;

  uint64_t end = now();

  return parseResult(parser, end - start);
}

////////////////////////////////////////////////////////////
//  Parse With String
///////////////////////////////////////////////////////////
template <typename... Args>
JSON::ParseResult parse(uint32_t &mask, PointerCursor cursor, Args &&...args) {
    return _parse(mask, cursor, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  Parse Top level array With Cursor
///////////////////////////////////////////////////////////
template <typename Cursor, typename T>
std::enable_if_t<is_cursor_v<Cursor> && is_derived_json_data_container_v<T>, JSON::ParseResult>
_parse(uint32_t &mask, Cursor& cursor, T &jsonObjects) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser.parse_array(jsonObjects);
  mask = parser.keyMask;

  uint64_t end = now();

  return parseResult(parser, end - start);
}

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, JSON::ParseResult>
parse(uint32_t &mask, PointerCursor cursor, T &jsonObjects) {
  return _parse(mask, cursor, jsonObjects);
}
NAMESPACE_JSON_END

JSON::ParseResult UnknownValueType::fromJSON(JSON::PointerCursor cursor) {
  uint32_t m = 0;
  return JSON::parse(m, cursor);
}

JSON::ParseResult JSONCallbackObject::fromJSON(JSON::PointerCursor cursor) {
  return JSON::parse(cursor, callback, array_index);
}

#ifdef ARDUINO
#include "StreamCursor.h"

JSON::ParseResult JSON::parse(StreamCursor cursor, const JSONCallback& cb, int arrayIndex) {
  return _parse(cursor, const_cast<const JSONCallback&>(cb), arrayIndex);
}

template <typename... Args>
JSON::ParseResult JSON::parse(uint32_t &mask, StreamCursor cursor, Args &&...args) {
    return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, JSON::ParseResult>
JSON::parse(uint32_t &mask, StreamCursor cursor, T &jsonObjects) {
  return _parse(mask, cursor, jsonObjects);
}

JSON::ParseResult UnknownValueType::fromJSON(JSON::StreamCursor& cursor) {
  uint32_t m = 0;
  return JSON::parse(m, cursor);
}

JSON::ParseResult JSONCallbackObject::fromJSON(JSON::StreamCursor& cursor) {
  return JSON::parse(cursor, callback, array_index);
}
#endif
