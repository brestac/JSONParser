#pragma once

#include <stddef.h>
#include <tuple>
#include <string_view>

NAMESPACE_JSON_BEGIN

template <typename Tuple>
constexpr inline Tuple _create_dispatch_tuple(Tuple& tuple) {
#if JSON_DEBUG_LEVEL > 0
  if (std::__is_constant_evaluated()) {
    #pragma message "create_dispatch_tuple is constant evaluated"

  } else {
    std::printf("create_dispatch_tuple is NOT constant evaluated\n");
  }
#endif
  
  return tuple;
}

template <typename Tuple, typename Value, typename... Args>
constexpr auto _create_dispatch_tuple(Tuple& tuple, std::string_view key, Value& value, Args&&... args) {

  auto pair = std::pair<std::string_view, decltype(value)>{key, value};

  auto new_tuple = std::tuple_cat(tuple, std::make_tuple(pair));

  return _create_dispatch_tuple(new_tuple, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr auto create_dispatch_tuple(Args&&... args) {
  std::tuple<> t;
  return _create_dispatch_tuple(t, std::forward<Args>(args)...);
}

template <size_t I, typename Tuple, typename Callback>
bool call_at(Tuple&& tuple, std::string_view search_key, Callback& callback) {
    auto& [key, value] = std::get<I>(tuple);
    if (key != search_key) return false;

    callback(value, I);

    return true;
}

template <typename Tuple, typename Callback, size_t... I>
void _dispatch_by_key(Tuple& tuple, std::string_view search_key, Callback& callback,
                       std::index_sequence<I...>) {
    (call_at<I>(tuple, search_key, callback) || ...);
}

template <typename Tuple, typename Callback>
void dispatch_by_key(Tuple& tuple, std::string_view search_key, Callback&& callback) {
  constexpr size_t N = std::tuple_size<remove_cv_ref_t<Tuple>>::value;
  _dispatch_by_key(tuple, search_key, callback, std::make_index_sequence<N>{});
}

NAMESPACE_JSON_END