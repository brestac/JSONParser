#pragma once

#include <string_view>
#include <vector>

#include "JSONParserBase.h"
#include "StringPool.h"
#include "StreamCursor.h"
#include "macros.h"
#include "utils.h"

NAMESPACE_JSON_BEGIN

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
std::enable_if_t<is_parser_type_v<Parser> && is_valid_args_v<Args...>, ParseResult> _parse_impl(uint32_t& mask, Parser& parser, Args &&...args) {
  using Cursor = decltype(parser.cursor());
  
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

  return NO_RESULT;
}

template <bool UseMask, typename TargetT, typename Cursor, typename... Args>
std::enable_if_t<is_cursor_v<Cursor>, ParseResult> _parse_impl(uint32_t& mask, Cursor& cursor, Args &&...args) {
  uint64_t start = now();
  JSONParserBase<Cursor, UseMask> parser(cursor);
  _parse_impl<UseMask, TargetT>(mask, parser, std::forward<Args>(args)...);
  uint64_t end = now();
  return ParseResult(&parser, end - start);
                        }

template <bool UseMask, typename TargetT, typename Stream, typename... Args>
std::enable_if_t<is_stream_v<Stream>, ParseResult> _parse_impl(uint32_t& mask, Stream& stream, Args &&...args) {
  StreamCursorReader cursor(stream);
  return _parse_impl<UseMask, TargetT>(mask, cursor, std::forward<Args>(args)...);
                        }

template <bool UseMask, typename TargetT, typename Buffer, typename... Args>
std::enable_if_t<is_buffer_v<Buffer>, ParseResult> _parse_impl(uint32_t& mask, Buffer& buffer, Args &&...args) {
  const PointerCursorReader cursor(buffer);
  return _parse_impl<UseMask, TargetT>(mask, cursor, std::forward<Args>(args)...);
                        }


////////////////////////////////////////////////////////////
//  parse — public API helpers
////////////////////////////////////////////////////////////
template <typename T, typename... Args>
ParseResult parse(uint32_t& mask, T& cursor, Args &&...args) {
  return _parse_impl<true, void>(mask, cursor, std::forward<Args>(args)...);
}

template <typename T>
ParseResult parse(T& input, const JSONCallback& cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT_KEY");
  uint32_t mask = 0;
  return _parse_impl<false, JSONCallbackObject>(mask, input, cb_obj);
}

////////////////////////////////////////////////////////////
//  parse — top-level array
////////////////////////////////////////////////////////////
template <typename T>
enable_if_json_data_container_compatible<T>
parse(uint32_t& mask, const PointerCursorReader& cursor, T& jsonObjects) {
  return _parse_impl<false, void>(mask, cursor, jsonObjects);
}

NAMESPACE_JSON_END

template <typename T>
JSON::ParseResult JSONCallbackObject::fromJSON(T& input) {
  uint32_t mask = 0;
  return _parse_impl<false, JSONCallbackObject>(mask, input, *this);
}