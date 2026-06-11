#pragma once

#include <string_view>
#include <vector>

#include "macros.h"
#include "JSONParserBase.h"
#include "StreamCursor.h"
#include "StaticString.h"
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
    std::enable_if_t<is_stream_v<T>, ParseResult>;

template <typename T>
using enable_if_json_data_container_compatible =
    std::enable_if_t<is_derived_json_data_container_v<T>, ParseResult>;

// Implémentation interne unique, UseMask en template bool direct
template <bool UseMask, typename Cursor, typename TargetT = Cursor, typename... Args>
ParseResult _parse_impl(const char* name, uint32_t& mask, Cursor& cursor, Args&&... args) {
    uint64_t start = now();

    JSONParserBase<Cursor, UseMask, TargetT> parser(name, cursor);

    JSON_DEBUG_COLOR(COLOR_RED, "Parser Cursor=%s TargetT=%s size=%zu\n", typeid(Cursor).name(), typeid(TargetT).name(), sizeof(parser));
    GLOBAL_PARSER_SIZE += sizeof(parser);
    if (GLOBAL_PARSER_SIZE > MAX_GLOBAL_PARSER_SIZE) {
        MAX_GLOBAL_PARSER_SIZE = GLOBAL_PARSER_SIZE;
    }

    if constexpr (std::is_same<remove_cvref_t<Cursor>, StreamCursor>::value && (sizeof... (Args) > 1)) {
        constexpr size_t n_sv = count_string_view_args_v<Args...>;
        StaticString<TargetT, MAX_STRING_POOL_REUSE_COUNT>::ensure_pool_size(n_sv);
    }

    if constexpr (UseMask) {
        const bool automask = are_generic_keys(std::forward<Args>(args)...);
        parser.setAutomask(automask);
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
enable_if_args_valid<Args...> _parse(const char* name, uint32_t& mask, Cursor& cursor, Args&&... args) {
    return _parse_impl<true>(name, mask, cursor, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  _parse — single argument (JSONCallbackObject ou UnknownValueType, no mask)
////////////////////////////////////////////////////////////
template <typename Cursor>
ParseResult _parse(const char* name, Cursor& cursor, JSONCallbackObject&cb ) {
    uint32_t mask = 0;
    return _parse_impl<false, Cursor, JSONCallbackObject>(name, mask, cursor, cb);
}

template <typename Cursor>
ParseResult _parse(const char* name, Cursor& cursor, UnknownValueType& unknown ) {
    uint32_t mask = 0;
    return _parse_impl<false, Cursor, UnknownValueType>(name, mask, cursor, unknown);
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
ParseResult parse(uint32_t &mask, const char* input, Args &&...args) {
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
  StreamCursor stream(input);
  return _parse("$ROOT", mask, stream, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  parse — callback
////////////////////////////////////////////////////////////

ParseResult parse(StreamCursor &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$CALLBACK");
  return _parse("$CALLBACK", cursor, cb_obj);
}

ParseResult parse(const PointerCursorReader &cursor, const JSONCallback &cb) {
  JSONCallbackObject cb_obj(cb, "$CALLBACK");
  return _parse("$CALLBACK", cursor, cb_obj);
}

////////////////////////////////////////////////////////////
//  parse — top-level array
////////////////////////////////////////////////////////////
template <typename T>
enable_if_json_data_container_compatible<T>
parse(uint32_t &mask, const PointerCursorReader &cursor, T &jsonObjects) {
  return _parse("$ROOT_ARRAY", mask, cursor, jsonObjects);
}

NAMESPACE_JSON_END

JSON::ParseResult UnknownValueType::fromJSON(const char* name, JSON::StreamCursor &cursor) {
  static UnknownValueType dummy;
  return JSON::_parse(name, cursor, dummy);
}

JSON::ParseResult UnknownValueType::fromJSON(const char* name, const JSON::PointerCursorReader &cursor) {
  static UnknownValueType dummy;
  return JSON::_parse(name, cursor, dummy);
}

JSON::ParseResult JSONCallbackObject::fromJSON(const char* name, const JSON::PointerCursorReader &cursor) {
  return JSON::_parse(name, cursor, *this);
}

JSON::ParseResult JSONCallbackObject::fromJSON(const char* name, JSON::StreamCursor &cursor) {
  return JSON::_parse(name, cursor, *this);
}
