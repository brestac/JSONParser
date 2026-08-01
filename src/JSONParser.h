#pragma once

#include <string_view>
#include <vector>

#include "JSONParserBase.h"
#include "StringPool.h"
#include "StreamCursor.h"
#include "macros.h"
#include "utils.h"

NAMESPACE_JSON_BEGIN

template <typename... Args>
using enable_if_args_valid =
    std::enable_if_t<key_value_checker_v<parsed_types, arguments_types,
                                         arguments_array_types, Args...>,
                     ParseResult>;

template <typename T>
using enable_if_cursor =
    std::enable_if_t<is_cursor_reader_v<remove_cv_ref_t<T>>, ParseResult>;

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
template <bool UseMask, typename TargetT, typename Parser, typename... Args>
std::enable_if_t<is_parser_type_v<Parser>, ParseResult> _parse_impl(uint32_t& mask, Parser& parser, Args &&...args) {
  using Cursor = decltype(parser.cursor());
  uint64_t start = now();
  
  JSON_DEBUG_COLOR(COLOR_RED, "Parser Cursor=%s TargetT=%s size=%zu\n",
                   typeid(Cursor).name(), typeid(TargetT).name(),
                   sizeof(parser));
#ifdef JSON_DEBUG_MEM
  GLOBAL_PARSER_SIZE += sizeof(parser);
  if (GLOBAL_PARSER_SIZE > MAX_GLOBAL_PARSER_SIZE) {
    MAX_GLOBAL_PARSER_SIZE = GLOBAL_PARSER_SIZE;
  }
#endif
  if constexpr (std::is_same<remove_cv_ref_t<Cursor>, StreamCursorReader>::value &&
                (sizeof...(Args) > 1)) {
    constexpr size_t n_sv = count_string_view_args_v<Args...>;
    StringPool<TargetT>::ensure_pool_size(n_sv);
  }

  if constexpr (UseMask) {
    const bool automask = are_generic_keys(std::forward<Args>(args)...);
    parser.setAutomask(automask);
  }

  parser.template parse<TargetT>(std::forward<Args>(args)...);

  if constexpr (UseMask) {
    mask = parser.keyMask();
  }

  uint64_t end = now();
  return ParseResult(&parser, end - start);
}

template <bool UseMask, typename TargetT, typename Cursor, typename... Args>
std::enable_if_t<is_cursor_v<Cursor>, ParseResult> _parse_impl(uint32_t& mask, Cursor& cursor, Args &&...args) {
  JSONParserBase<Cursor, UseMask> parser(cursor);
  return _parse_impl<UseMask, TargetT>(mask, parser, std::forward<Args>(args)...);
                        }
////////////////////////////////////////////////////////////
//  _parse — key-value args (mask tracked)
////////////////////////////////////////////////////////////
template <typename Parser, typename... Args>
enable_if_args_valid<Args...> _parse(uint32_t& mask,
                                     Parser& parser, Args &&...args) {
  return _parse_impl<true, void>(mask, parser, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  _parse — single argument (JSONCallbackObject ou UnknownValueType, no mask)
////////////////////////////////////////////////////////////
template <typename Cursor>
ParseResult _parse(Cursor& cursor, JSONCallbackObject& cb) {
  uint32_t mask = 0;
  return _parse_impl<false, JSONCallbackObject>(mask, cursor, cb);
}

template <typename Cursor>
ParseResult _parse(Cursor& cursor, UnknownValueType& unknown) {
  uint32_t mask = 0;
  return _parse_impl<false, UnknownValueType>(mask, cursor,
                                                      unknown);
}

////////////////////////////////////////////////////////////
//  parse — public API helpers
////////////////////////////////////////////////////////////
template <typename... Args>
ParseResult parse(uint32_t& mask, const PointerCursorReader& cursor,
                  Args &&...args) {
  return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <typename... Args>
ParseResult parse(uint32_t& mask, StreamCursorReader& stream, Args &&...args) {
  return _parse(mask, stream, std::forward<Args>(args)...);
}

template <typename... Args>
ParseResult parse(uint32_t& mask, const char *input, Args &&...args) {
  const PointerCursorReader cursor(input,
                                   str_length(input, MAX_JSON_LENGTH));
  return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <size_t N, typename... Args>
ParseResult parse(uint32_t& mask, const char (&input)[N], Args &&...args) {
  const PointerCursorReader cursor(input, N - 1);
  return _parse(mask, cursor, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
enable_if_stream_compatible<T> parse(uint32_t& mask, T& input, Args &&...args) {
  StreamCursorReader stream(input);
  return _parse(mask, stream, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  parse — callback
////////////////////////////////////////////////////////////

ParseResult parse(StreamCursorReader& cursor, const JSONCallback& cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT_KEY");
  return _parse(cursor, cb_obj);
}

ParseResult parse(const PointerCursorReader& cursor, const JSONCallback& cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT_KEY");
  return _parse(cursor, cb_obj);
}

////////////////////////////////////////////////////////////
//  parse — top-level array
////////////////////////////////////////////////////////////
template <typename T>
enable_if_json_data_container_compatible<T>
parse(uint32_t& mask, const PointerCursorReader& cursor, T& jsonObjects) {
  return _parse(mask, cursor, jsonObjects);
}

NAMESPACE_JSON_END

JSON::ParseResult UnknownValueType::fromJSON(JSON::StreamCursorReader& cursor) {
  static UnknownValueType dummy;
  uint32_t mask = 0;
  return _parse_impl<false, UnknownValueType>(mask, cursor, dummy);
}

JSON::ParseResult
UnknownValueType::fromJSON(const JSON::PointerCursorReader& cursor) {
  static UnknownValueType dummy;
  uint32_t mask = 0;
  return _parse_impl<false, UnknownValueType>(mask, cursor, dummy);
}
 
template <typename Parser>
JSON::ParseResult UnknownValueType::_fromJSON(Parser& parser) {
  uint32_t mask = 0;
  return _parse_impl<false, UnknownValueType>(mask, parser, *this);
}

JSON::ParseResult
JSONCallbackObject::fromJSON(const JSON::PointerCursorReader& cursor) {
  uint32_t mask = 0;
  return _parse_impl<false, JSONCallbackObject>(mask, cursor, *this);
}

JSON::ParseResult JSONCallbackObject::fromJSON(JSON::StreamCursorReader& cursor) {
  uint32_t mask = 0;
  return _parse_impl<false, JSONCallbackObject>(mask, cursor, *this);
}

template <typename Parser>
JSON::ParseResult JSONCallbackObject::_fromJSON(Parser& parser) {
  uint32_t mask = 0;
  return _parse_impl<false, JSONCallbackObject>(mask, parser, *this);
}