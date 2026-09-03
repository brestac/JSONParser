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
template <bool UseMask, typename TargetT, typename Cursor, typename M, typename... Args>
ParseResult _parse_impl(M&& mask, JSONParserBase<Cursor, UseMask>* parser, Args&&... args) {
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
  if constexpr (is_stream_cursor_reader_v<Cursor> && !is_callback<TargetT>) {
    constexpr size_t count = count_string_views<Args...>;
    StringPool<TargetT>::ensure_pool_size(count);
  }

  // if constexpr (is_dispatch_info_v<Arg> && UseMask) {
  //   parser->setAutomask(arg.is_generic_keys);
  // }
  parser->template parse<TargetT>(std::forward<Args>(args)...);

  if constexpr (UseMask) {
    mask = static_cast<MaskType>(parser->keyMask());
  }

  return NO_RESULT;
}

template <bool UseMask, typename TargetT, typename M, typename Cursor, typename... Args>
constexpr std::enable_if_t<is_cursor_v<Cursor>, ParseResult> _parse_impl(M&& mask, Cursor& cursor, Args&&... args) {
  uint64_t start = now();
  JSONParserBase<Cursor, UseMask> parser(cursor);
  _parse_impl<UseMask, TargetT, Cursor>(mask, &parser, std::forward<Args>(args)...);
  return ParseResult(&parser, now() - start);
}

template <bool UseMask, typename TargetT, typename M, typename Stream, typename... Args>
constexpr std::enable_if_t<is_stream_v<Stream>, ParseResult> _parse_impl(M&& mask, Stream& stream, Args&&... args) {
  StreamCursorReader cursor(stream);
  return _parse_impl<UseMask, TargetT>(mask, cursor, std::forward<Args>(args)...);
}

template <bool UseMask, typename TargetT, typename M, typename Buffer, typename... Args>
constexpr std::enable_if_t<is_buffer_v<Buffer>, ParseResult> _parse_impl(M&& mask, Buffer& buffer, Args&&... args) {
  const PointerCursorReader cursor(buffer);
  return _parse_impl<UseMask, TargetT>(mask, cursor, std::forward<Args>(args)...);
}

template <bool UseMask, typename TargetT, typename M, typename... Args>
constexpr ParseResult _parse_impl(M&& mask, const char* buffer, size_t size, Args&&... args) {
  const PointerCursorReader cursor(buffer, size);
  return _parse_impl<UseMask, TargetT>(mask, cursor, std::forward<Args>(args)...);
}

////////////////////////////////////////////////////////////
//  parse — public API
////////////////////////////////////////////////////////////

// parse — top-level object
template <typename M, typename T, typename... Args>
std::enable_if_t<(sizeof...(Args) > 1), ParseResult>
parse(M&& mask, T& input, Args&& ...args) {
  return _parse_impl<true, void>(mask, input, std::forward<Args>(args)...);
}

// template <typename M, typename T, typename Tuple>
// std::enable_if_t<is_tuple_v<Tuple>, ParseResult>
// parse(M&& mask, T& input, Tuple&& tuple) {
//   return _parse_impl<true, void>(mask, input, tuple);
// }

// Parse — top-level array of objects
template <typename T, typename C>
std::enable_if_t<container_info<C>::is_container, ParseResult>
parse(T& input, C& jsonObjects) {
  return _parse_impl<true, C>(0, input, "$", jsonObjects);
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
JSON::ParseResult JSONCallbackObject::_fromJSON(T* parser) {
  return _parse_impl<false, JSONCallbackObject>(0, parser, *this);
}