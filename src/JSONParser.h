#pragma once

#include <string_view>

#include "JSONParserBase.h"
#include "StringPool.h"
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

// Implémentation interne unique, UseMask en template bool direct
template <bool UseMask, typename TargetT, typename Cursor, typename M, typename Arg>
ParseResult _parse_impl(M&& mask, JSONParserBase<Cursor, UseMask>* parser, Arg& arg) {
  using MaskType = std::remove_reference_t<M>;
  
  static_assert(std::is_integral<MaskType>::value, "Mask must be an integral type");
  
  JSON_DEBUG_COLOR(COLOR_RED, "Parser Cursor=%s TargetT=%s size=%zu\n",
                   typeid(Cursor).name(), typeid(TargetT).name(),
                   sizeof(parser));
#ifdef JSON_DEBUG_MEM
  GLOBAL_PARSER_SIZE += sizeof(parser);
  if (GLOBAL_PARSER_SIZE > MAX_GLOBAL_PARSER_SIZE) {
    MAX_GLOBAL_PARSER_SIZE = GLOBAL_PARSER_SIZE;
  }
#endif
  // Si le curseur est un StreamCursor et que la cible est une structure avec
  // des string_view, on doit allouer un pool de mémoire pour les string_view
  if constexpr (is_dispatch_info_v<Arg> && is_stream_cursor_reader_v<Cursor> && !is_callback<TargetT>) {
    StringPool<TargetT>::ensure_pool_size(arg.sv_count);
  }
  
  if constexpr (is_dispatch_info_v<Arg> && UseMask) {
    parser->setAutomask(arg.is_generic_keys);
  }

  parser->template parse<TargetT>(arg);

  if constexpr (UseMask) {
    mask = static_cast<MaskType>(parser->keyMask());
  }

  return NO_RESULT;
}

template <bool UseMask, typename TargetT, typename M, typename Cursor, typename Arg>
std::enable_if_t<is_cursor_v<Cursor>, ParseResult> _parse_impl(M&& mask, Cursor& cursor, Arg&& arg) {
  uint64_t start = now();
  JSONParserBase<Cursor, UseMask> parser(cursor);
  _parse_impl<UseMask, TargetT, Cursor>(mask, &parser, arg);
  return ParseResult(&parser, now() - start);
}

template <bool UseMask, typename TargetT, typename M, typename Stream, typename Arg>
std::enable_if_t<is_stream_v<Stream>, ParseResult> _parse_impl(M&& mask, Stream& stream, Arg&& arg) {
  StreamCursorReader cursor(stream);
  return _parse_impl<UseMask, TargetT>(mask, cursor, arg);
}

template <bool UseMask, typename TargetT, typename M, typename Buffer, typename Arg>
std::enable_if_t<is_buffer_v<Buffer>, ParseResult> _parse_impl(M&& mask, Buffer& buffer, Arg&& arg) {
  const PointerCursorReader cursor(buffer);
  return _parse_impl<UseMask, TargetT>(mask, cursor, arg);
}

template <bool UseMask, typename TargetT, typename M, typename Arg>
ParseResult _parse_impl(M&& mask, const char* buffer, size_t size, Arg&& arg) {
  const PointerCursorReader cursor(buffer, size);
  return _parse_impl<UseMask, TargetT>(mask, cursor, arg);
}

////////////////////////////////////////////////////////////
//  parse — public API helpers
////////////////////////////////////////////////////////////

// parse — top-level object
template <typename M, typename T, typename... Args>
ParseResult parse(M&& mask, T& input, Args &&...args) {
  auto dispatch_table = create_dispatch_table( std::forward<Args>( args )... );
  return _parse_impl<true, void>(mask, input, dispatch_table);
}

// Parse — top-level array of objects
template <typename T, typename C>
std::enable_if_t<is_derived_json_data_container_v<C>, ParseResult>
parse(T& input, C& jsonObjects) {
  return _parse_impl<true, C>(0, input, jsonObjects);
}

// Parse — top-level object with callback
template <typename T>
ParseResult parse(T& input, const JSONCallback& cb) {
  JSONCallbackObject cb_obj(cb, "$ROOT_KEY");
  return _parse_impl<false, JSONCallbackObject>(0, input, cb_obj);
}

NAMESPACE_JSON_END

template <typename T>
JSON::ParseResult JSONCallbackObject::fromJSON(T& input) {
  return _parse_impl<false, JSONCallbackObject>(0, input, *this);
}

template <typename T>
JSON::ParseResult JSONCallbackObject::fromJSON(T* input) {
  return _parse_impl<false, JSONCallbackObject>(0, input, *this);
}