#pragma once

#include <string_view>

#include "macros.h"
#include "JSONStreamParser.h"
#include "StreamCursor.h"
#include "utils.h"

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
// Parse avec mask — convertit les const char[N] en JSONKey mémoïsées
template <typename Cursor, typename... Args>
enable_if_args_valid<Args...> _parse(std::string_view name, uint32_t& mask,
                                     Cursor& cursor, Args&&... args) {
    uint64_t start = now();

    // Reset du string pool uniquement pour le parser racine (bytesConsumed==0)
    // Les parsers enfants partagent le même curseur et ne doivent pas resetter.
    // if constexpr (std::is_same_v<remove_cvref_t<Cursor>, StreamCursor>) {
    //     if (cursor.bytesConsumed() == 0) {
    //         cursor.reset_string_pool();
    //     }
    // }

    JSONParserBase<Cursor> parser(name, cursor);
    bool automask = are_generic_keys(std::forward<Args>(args)...);
    parser.setAutomask(automask);

    parser.parse(std::forward<Args>(args)...);

    mask = parser.keyMask();

    uint64_t end = now();
    return ParseResult(&parser, end - start);
}

// Parse sans mask (JSONCallbackObject ou UnknownValueType)
template <typename Cursor, typename... Args>
ParseResult __parse(std::string_view name, Cursor& cursor, Args&&... args) {
    uint64_t start = now();

    // if constexpr (std::is_same_v<remove_cvref_t<Cursor>, StreamCursor>) {
    //     if (cursor.bytesConsumed() == 0) {
    //         cursor.reset_string_pool();
    //     }
    // }

    JSONParserBase<Cursor> parser(name, cursor);
    parser.setUseMask(false);

    parser.parse(std::forward<Args>(args)...);

    uint64_t end = now();
    return ParseResult(&parser, end - start);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, const PointerCursorReader &cursor, Args &&...args) {
  return _parse("$ROOT", mask, cursor, std::forward<Args>(args)...);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, StreamCursor &stream, Args &&...args) {
  return _parse("$ROOT", mask, stream, std::forward<Args>(args)...);
}

template <typename... Args>
ParseResult parse(uint32_t &mask, const char *input, Args &&...args) {
  const PointerCursorReader cursor(input, str_length(input, MAX_POINTER_CURSOR_SIZE));
  return _parse("$ROOT", mask, cursor, std::forward<Args>(args)...);
}

template <size_t N, typename... Args>
ParseResult parse(uint32_t &mask, const char (&input)[N], Args &&...args) {
  const PointerCursorReader cursor(input, N - 1);
  return _parse("$ROOT", mask, cursor, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
enable_if_stream_compatible<T> parse(uint32_t &mask, T &input, Args &&...args) {
  //constexpr size_t pool_size = sizeof...(Args) / 2 * MAX_KEY_LENGTH;
  StreamCursor stream(input);
  return _parse("$ROOT", mask, stream, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  Parse With Callback
///////////////////////////////////////////////////////////

ParseResult parse(StreamCursor &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT");
  return __parse("$ROOT", cursor, cb_obj);
}

ParseResult parse(const PointerCursorReader &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT");
  return __parse("$ROOT", cursor, cb_obj);
}

////////////////////////////////////////////////////////////
//  Parse Top level array
///////////////////////////////////////////////////////////
template <typename T>
enable_if_json_data_container_compatible<T>
parse(uint32_t &mask, const PointerCursorReader &cursor, T &jsonObjects) {
  return _parse("$ROOT", mask, cursor, jsonObjects);
}

// template <typename T>
// std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>
// parse(uint32_t &mask, StreamCursor& cursor, T &jsonObjects) {
//   return _parse(mask, cursor, jsonObjects);
// }

NAMESPACE_JSON_END

JSON::ParseResult UnknownValueType::fromJSON(std::string_view name, JSON::StreamCursor &cursor) {
  static UnknownValueType dummy;
  return JSON::__parse(name, cursor, dummy);
}

JSON::ParseResult UnknownValueType::fromJSON(std::string_view name, const JSON::PointerCursorReader &cursor) {
    static UnknownValueType dummy;
  return JSON::__parse(name, cursor, dummy);
}

JSON::ParseResult JSONCallbackObject::fromJSON(std::string_view name, const JSON::PointerCursorReader &cursor) {
  return JSON::__parse(name, cursor, *this);
}

JSON::ParseResult JSONCallbackObject::fromJSON(std::string_view name, JSON::StreamCursor &cursor) {
  return JSON::__parse(name, cursor, *this);
}

// #endif
