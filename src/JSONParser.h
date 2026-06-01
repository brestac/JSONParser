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

// Implémentation interne unique, UseMask en template bool direct
template <bool UseMask, typename Cursor, typename... Args>
ParseResult _parse_impl(std::string_view name, uint32_t& mask, Cursor& cursor, Args&&... args) {
    uint64_t start = now();

    JSONParserBase<Cursor> parser(name, cursor);

    if constexpr (UseMask) {
        const bool automask = are_generic_keys(std::forward<Args>(args)...);
        parser.setAutomask(automask);
        parser.setUseMask(true);
    } else {
        parser.setUseMask(false);
    }

    parser.parse(std::forward<Args>(args)...);

    if constexpr (UseMask) {
        mask = parser.keyMask();
    }

    uint64_t end = now();
    return ParseResult(&parser, end - start);
}

////////////////////////////////////////////////////////////
//  _parse — key-value args (mask tracked)
////////////////////////////////////////////////////////////
template <typename Cursor, typename... Args>
enable_if_args_valid<Args...> _parse(std::string_view name, uint32_t& mask, Cursor& cursor, Args&&... args) {
    return _parse_impl<true>(name, mask, cursor, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  _parse — single argument (JSONCallbackObject ou UnknownValueType, no mask)
////////////////////////////////////////////////////////////
template <typename Cursor, typename... Args>
ParseResult _parse(std::string_view name, Cursor& cursor, Args&&... args) {
    uint32_t mask = 0;
    return _parse_impl<false>(name, mask, cursor, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  parse — public API helpers
////////////////////////////////////////////////////////////
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
  // pool_limit calculé à la compilation : n_string_views * MAX_VALUE_LENGTH
  constexpr size_t n_sv      = count_string_view_args_v<Args...>;
  constexpr size_t pool_limit = (n_sv == 0)
      ? JSON::STREAM_STRING_POOL_SIZE
      : n_sv * JSON::MAX_VALUE_LENGTH;
  StreamCursor stream(input, pool_limit);
  return _parse("$ROOT", mask, stream, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  parse — callback
////////////////////////////////////////////////////////////

ParseResult parse(StreamCursor &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT");
  return _parse("$ROOT", cursor, cb_obj);
}

ParseResult parse(const PointerCursorReader &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT");
  return _parse("$ROOT", cursor, cb_obj);
}

////////////////////////////////////////////////////////////
//  parse — top-level array
////////////////////////////////////////////////////////////
template <typename T>
enable_if_json_data_container_compatible<T>
parse(uint32_t &mask, const PointerCursorReader &cursor, T &jsonObjects) {
  return _parse("$ROOT", mask, cursor, jsonObjects);
}

NAMESPACE_JSON_END

JSON::ParseResult UnknownValueType::fromJSON(std::string_view name, JSON::StreamCursor &cursor) {
  static UnknownValueType dummy;
  return JSON::_parse(name, cursor, dummy);
}

JSON::ParseResult UnknownValueType::fromJSON(std::string_view name, const JSON::PointerCursorReader &cursor) {
  static UnknownValueType dummy;
  return JSON::_parse(name, cursor, dummy);
}

JSON::ParseResult JSONCallbackObject::fromJSON(std::string_view name, const JSON::PointerCursorReader &cursor) {
  return JSON::_parse(name, cursor, *this);
}

JSON::ParseResult JSONCallbackObject::fromJSON(std::string_view name, JSON::StreamCursor &cursor) {
  return JSON::_parse(name, cursor, *this);
}
