#pragma once

#include "JSONStreamParser.h"
#include "JSONPrinter.h"
#include "macros.h"

NAMESPACE_JSON_BEGIN

template <typename Cursor>
ParseResult resultForParser(JSONParserBase<Cursor> &parser, uint64_t duration) {
  return ParseResult(parser.parsed_length(), parser.nKeys, parser.nParsed, parser.nConverted, parser.nUpdated, parser.error(), duration, parser._state == JSONParserBase<Cursor>::ParserState::STOPPED);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, const PointerCursorReader& cursor, Args &&...args);

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>
parse(uint32_t &mask, const PointerCursorReader& cursor, T &jsonObjects);

ParseResult parse(const PointerCursorReader &cursor, const JSONCallback& cb, int arrayIndex = -1);

#ifdef ARDUINO
ParseResult parse(StreamCursor& cursor, const JSONCallback& cb, int arrayIndex = -1);

template <typename... Args>
ParseResult parse(uint32_t &mask, StreamCursor& cursor, Args &&...args);

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>
parse(uint32_t &mask, StreamCursor& cursor, T &jsonObjects);
#endif

template <typename Cursor>
std::enable_if_t<is_cursor_reader_v<Cursor>, ParseResult>
_parse(Cursor& cursor, const JSONCallback &cb, int arrayIndex);
  
template <typename Cursor, typename... Args>
std::enable_if_t<is_cursor_reader_v<Cursor> && key_value_checker_v<parsed_types, arguments_types, arguments_array_types, Args...>, ParseResult>
_parse(uint32_t &mask, Cursor& cursor, Args &&...args);
  
template <typename Cursor, typename T>
std::enable_if_t<is_cursor_reader_v<Cursor> && is_derived_json_data_container_v<T>, ParseResult>
_parse(uint32_t &mask, Cursor& cursor, T &jsonObjects);

////////////////////////////////////////////////////////////
//  Parse With Callback
///////////////////////////////////////////////////////////
template <typename Cursor>
std::enable_if_t<is_cursor_reader_v<Cursor>, ParseResult>
_parse(Cursor& cursor, const JSONCallback &cb, int arrayIndex) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser.setArrayIndex(arrayIndex);
  parser.parse(cb);

  uint64_t end = now();

  return resultForParser(parser, end - start);
}

////////////////////////////////////////////////////////////
//  Parse With Cursor
///////////////////////////////////////////////////////////
template <typename Cursor, typename... Args>
std::enable_if_t<is_cursor_reader_v<Cursor> && key_value_checker_v<parsed_types, arguments_types, arguments_array_types, Args...>, ParseResult>
_parse(uint32_t &mask, Cursor& cursor, Args &&...args) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser._automask = are_keys(std::forward<Args>(args)...);
  parser.parse(std::forward<Args>(args)...);
  mask = parser.keyMask;

  uint64_t end = now();

  return resultForParser(parser, end - start);
}

////////////////////////////////////////////////////////////
//  Parse Top level array With Cursor
///////////////////////////////////////////////////////////
template <typename Cursor, typename T>
std::enable_if_t<is_cursor_reader_v<Cursor> && is_derived_json_data_container_v<T>, JSON::ParseResult>
_parse(uint32_t &mask, Cursor& cursor, T &jsonObjects) {
  uint64_t start = now();

  JSONParserBase<Cursor> parser(cursor);
  parser.parse_array(jsonObjects);
  mask = parser.keyMask;

  uint64_t end = now();

  return resultForParser(parser, end - start);
}

NAMESPACE_JSON_END

#ifdef ARDUINO
#include "StreamCursor.h"

JSON::ParseResult UnknownValueType::fromJSON(JSON::StreamCursor& cursor) {
  uint32_t m = 0;
  return JSON::_parse(m, cursor);
}

JSON::ParseResult JSONCallbackObject::fromJSON(JSON::StreamCursor& cursor) {
  return JSON::_parse(cursor, this->callback, this->array_index);
}

JSON::ParseResult JSON::parse(StreamCursor& cursor, const JSONCallback& cb, int arrayIndex) {
  return JSON::_parse(cursor, const_cast<const JSONCallback&>(cb), arrayIndex);
}

template <typename... Args>
JSON::ParseResult JSON::parse(uint32_t &mask, StreamCursor& cursor, Args &&...args) {
    return JSON::_parse(mask, cursor, std::forward<Args>(args)...);
}

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, JSON::ParseResult>
JSON::parse(uint32_t &mask, StreamCursor& cursor, T &jsonObjects) {
  return JSON::_parse(mask, cursor, jsonObjects);
}

#else

JSON::ParseResult UnknownValueType::fromJSON(const JSON::PointerCursorReader& cursor) {
  uint32_t m = 0;
  PointerCursorReader c = cursor;
  return JSON::_parse(m, c);
}

JSON::ParseResult JSONCallbackObject::fromJSON(const JSON::PointerCursorReader& cursor) {
  PointerCursorReader c = cursor;
  return JSON::_parse(c, this->callback, this->array_index);
}

template <typename... Args>
JSON::ParseResult JSON::parse(uint32_t &mask, const PointerCursorReader &cursor, Args &&...args) {
  PointerCursorReader c = cursor;
  return JSON::_parse(mask, c, std::forward<Args>(args)...);
}

template <typename T>
std::enable_if_t<is_derived_json_data_container_v<T>, JSON::ParseResult>
JSON::parse(uint32_t &mask, const PointerCursorReader& cursor, T &jsonObjects) {
  PointerCursorReader c = cursor;
  return JSON::_parse(mask, c, jsonObjects);
}

JSON::ParseResult JSON::parse(const PointerCursorReader& cursor, const JSONCallback& cb, int arrayIndex) {
  PointerCursorReader c = cursor;
  return JSON::_parse(c, cb, arrayIndex);
}

#endif
