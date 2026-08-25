#pragma once

#ifdef __EXCEPTIONS
#include <stdexcept>
#include <typeinfo>
#endif

#include <array>
#include <vector>
#include <list>
#include <stdint.h>
#include <string_view>
#include <type_traits>
#include <variant>

#include "constants.h"
#include "macros.h"

// template <typename Cursor, bool UseMask>
// class JSONParserBase;

struct JSONObject;
struct JSONKey;
struct JSONCallbackObject;
struct ParseValueResult;

using namespace JSON;
// ---------------------------------------------------------------------------
//   Type checker
// ---------------------------------------------------------------------------

// template <size_t N, class... Args> decltype(auto) getNthArg(Args &&...args) {
//   static_assert(N < sizeof...(Args), "Index out of bounds");

//   return std::get<N>(std::forward_as_tuple(std::forward<Args>(args)...));
// }

template <class... Args> constexpr bool args_are_pairs = (sizeof...(Args) > 0) && (sizeof...(Args) % 2) == 0;

template <class... Args> constexpr bool args_not_empty = sizeof...(Args) > 0;

template <typename T, typename From, typename = void> struct is_static_castable_from : std::false_type {};

template <typename T, typename From>
struct is_static_castable_from<T, From, std::void_t<decltype(static_cast<T>(std::declval<From>()))>> : std::true_type {
};

template <typename T, typename TypeList> struct is_castable_from_any;

template <typename T, typename... Ts>
struct is_castable_from_any<T, type_list<Ts...>> : std::disjunction<is_static_castable_from<T, Ts>...> {};

template <typename T, typename TypeList> struct is_in_type_list : std::false_type {};

template <typename T, typename... Ts>
struct is_in_type_list<T, type_list<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {};

// Type de container
enum class ContainerKind { NOT_CONTAINER, C_ARRAY, CHAR_ARRAY, STD_ARRAY, STD_VECTOR, STD_LIST };

// primary template
template <typename T> struct container_info {
  using base_t = T;
  using base_container_t = void;                    // pas un container
  using child_t = void;
  static constexpr size_t dimensions = 0;
  static constexpr ContainerKind kind = ContainerKind::NOT_CONTAINER;
  static constexpr size_t extent = 0;
  static constexpr bool is_container = false;
  static constexpr bool fixed = false;
};

// C-array
template <typename T, size_t N> struct container_info<T[N]> {
  using base_t = typename container_info<T>::base_t;
  using base_container_t = std::conditional_t<container_info<T>::is_container,
                                             typename container_info<T>::base_container_t,
                                             T[N]>;
  using child_t = T;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::C_ARRAY;
  static constexpr size_t extent = N;
  static constexpr bool is_container = true;
  static constexpr bool fixed = true;
};

// Char array (inchangé, ce n'est pas un container)
template <size_t N> struct container_info<char[N]> {
  using base_t = char;
  using base_container_t = void;
  using child_t = char;
  static constexpr size_t dimensions = 0;
  static constexpr ContainerKind kind = ContainerKind::CHAR_ARRAY;
  static constexpr size_t extent = N;
  static constexpr bool is_container = false;
  static constexpr bool fixed = true;
};

// std::array
template <typename T, size_t N> struct container_info<std::array<T, N>> {
  using base_t = typename container_info<T>::base_t;
  using base_container_t = std::conditional_t<container_info<T>::is_container,
                                             typename container_info<T>::base_container_t,
                                             std::array<T, N>>;
  using child_t = T;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::STD_ARRAY;
  static constexpr size_t extent = N;
  static constexpr bool is_container = true;
  static constexpr bool fixed = true;
};

// std::vector
template <typename T, typename A> struct container_info<std::vector<T, A>> {
  using base_t = typename container_info<T>::base_t;
  using base_container_t = std::conditional_t<container_info<T>::is_container,
                                             typename container_info<T>::base_container_t,
                                             std::vector<T, A>>;
  using child_t = T;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::STD_VECTOR;
  static constexpr size_t extent = MAX_ARRAY_LENGTH;
  static constexpr bool is_container = true;
  static constexpr bool fixed = false;
};

// std::list
template <typename T, typename A> struct container_info<std::list<T, A>> {
  using base_t = typename container_info<T>::base_t;
  using base_container_t = std::conditional_t<container_info<T>::is_container,
                                             typename container_info<T>::base_container_t,
                                             std::list<T, A>>;
  using child_t = T;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::STD_LIST;
  static constexpr size_t extent = MAX_ARRAY_LENGTH;
  static constexpr bool is_container = true;
  static constexpr bool fixed = false;
};

template <typename T>
using base_container_t = typename container_info<T>::base_container_t;

template <typename T> constexpr bool is_container_v = container_info<T>::is_container;

template <typename T>
constexpr bool is_vector_v = is_container_v<T> && container_info<T>::kind == ContainerKind::STD_VECTOR;

template <typename T>
constexpr bool is_array_v = is_container_v<T> && container_info<T>::kind == ContainerKind::STD_ARRAY;

template <typename T>
constexpr bool is_c_array_v = is_container_v<T> && container_info<T>::kind == ContainerKind::C_ARRAY;

// ==========================================
// Container from list checker
// ==========================================

// Check if a type T is a container of a type in TypeList. A container needs to have container_info<T>::is_container == true
// and the base type of the container needs to be in TypeList.
template <typename T, typename TypeList>
struct is_container_from_list : std::integral_constant<bool, 
                                  container_info<T>::is_container &&
                                  is_in_type_list<typename container_info<T>::base_t, TypeList>::value
                                > {};

// ==========================================
// char array, char array array
// ==========================================

template <typename T> struct is_char_array : std::false_type {};

template <typename T, size_t N> struct is_char_array<T[N]> : std::is_same<std::remove_cv_t<T>, char> {};

template <typename T> inline constexpr bool is_char_array_v = is_char_array<T>::value;

template <typename T> struct is_char_array_array : std::false_type {};

template <typename T, size_t N, size_t M>
struct is_char_array_array<T[N][M]> : std::integral_constant<bool, std::is_same_v<T, char>> {};

template <typename T> inline constexpr bool is_char_array_array_v = is_char_array_array<T>::value;

// ==========================================
// JSONObject
// ==========================================
template <typename T> struct is_derived_json_data : std::is_base_of<JSONObject, remove_cv_ref_t<T>> {};

template <typename T> struct is_derived_json_data<T *> : is_derived_json_data<T> {};

template <typename T> inline constexpr bool is_derived_json_data_v = is_derived_json_data<remove_cv_ref_t<T>>::value;

template <typename T>
constexpr bool is_derived_json_data_container_v =
    container_info<T>::is_container && is_derived_json_data<typename container_info<T>::base_t>::value;

// ==========================================
// Cursor
// ==========================================

template <typename T, typename = void> struct is_cursor_reader : std::false_type {};

template <typename T, typename = void> struct is_cursor_reader_constructible : std::false_type {};

template <typename T, typename = void> struct is_cursor_writer : std::false_type {};

template <typename T, typename = void> struct is_cursor_writer_constructible : std::false_type {};

template <> struct is_cursor_reader<const JSON::PointerCursorReader> : std::true_type {};

template <> struct is_cursor_reader<JSON::StreamCursorReader> : std::true_type {};

template <typename T>
struct is_cursor_reader_constructible<T> : std::is_constructible<T, const JSON::PointerCursorReader> {};

template <> struct is_cursor_writer<JSON::PointerCursorWriter> : std::true_type {};

template <> struct is_cursor_writer<JSON::StreamCursorWriter> : std::true_type {};

template <typename T>
struct is_cursor_writer_constructible<T> : std::is_constructible<T, const JSON::PointerCursorWriter> {};

template <typename T> inline constexpr bool is_cursor_reader_v = is_cursor_reader<T>::value;

template <typename T> inline constexpr bool is_cursor_reader_constructible_v = is_cursor_reader_constructible<T>::value;

template <typename T> inline constexpr bool is_cursor_writer_v = is_cursor_writer<T>::value;

template <typename T> inline constexpr bool is_cursor_writer_constructible_v = is_cursor_writer_constructible<T>::value;

template <typename T> inline constexpr bool is_cursor_v = is_cursor_reader_v<T> || is_cursor_writer_v<T>;

template <typename T> inline constexpr bool is_stream_v = std::is_base_of<Stream, remove_cv_ref_t<std::remove_pointer_t<T>>>::value;

template <typename T> inline constexpr bool is_buffer_v = std::is_constructible_v<JSON::PointerCursorReader, T> || std::is_same_v<remove_cv_ref_t<T>, std::string>;

template <typename T> inline constexpr bool is_pointer_cursor_reader_v = std::is_same_v<remove_cv_ref_t<T>, JSON::PointerCursorReader>;

template <typename T> inline constexpr bool is_stream_cursor_reader_v = std::is_same_v<remove_cv_ref_t<T>, JSON::StreamCursorReader>;

// ==========================================
// Key Value checker
// ==========================================
// template <typename T, typename = void> struct is_convertible_to_indexed_key : std::false_type {};

// template <typename T>
// struct is_convertible_to_indexed_key<T, std::void_t<decltype(JSONIndexedKey(std::declval<T>()))>> : std::true_type {};
#if !defined(DISABLE_ARGS_CHECK)

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename Value>
struct value_checker
    : std::disjunction<is_castable_from_any<remove_cv_ref_t<Value>, CastableTypeList>,
                       is_in_type_list<remove_cv_ref_t<Value>, TypeList>,
                       is_container_from_list<remove_cv_ref_t<Value>, ArrayTypeList>,
                       is_char_array<remove_cv_ref_t<Value>>, is_char_array_array<remove_cv_ref_t<Value>>,
                       is_derived_json_data<remove_cv_ref_t<Value>>, std::is_pointer<remove_cv_ref_t<Value>>,
                       std::integral_constant<bool, is_derived_json_data_container_v<remove_cv_ref_t<Value>>>> {};

template <typename Key> struct key_checker : std::is_constructible<JSONKey, Key> {};

template <typename CastableTypeList, typename ArrayTypeList, typename... Args> struct key_value_checker;

template <typename CastableTypeList, typename TypeList,
          /*typename ArrayTypeList,*/ typename... Args>
struct key_value_checker<CastableTypeList, TypeList,
                         /*ArrayTypeList,*/ type_list<Args...>> : std::true_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename T>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList /*, ArrayArrayTypeList*/, T> : std::false_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList/*,
          typename ArrayArrayTypeList*/>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList/*, ArrayArrayTypeList*/>
    : std::true_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename Key, typename Value, class... Rest>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList,
                         /*ArrayArrayTypeList,*/ Key, Value, Rest...>
    : std::conjunction<key_checker<Key>,
                       value_checker<CastableTypeList, TypeList, ArrayTypeList,
                                     /*ArrayArrayTypeList,*/ Value>,
                       key_value_checker<CastableTypeList, TypeList, ArrayTypeList /*, ArrayArrayTypeList*/, Rest...>> {
};

template <class... Args> constexpr bool arg_is_valid = false;

template <class Arg>
constexpr bool arg_is_valid<Arg> =
    std::is_same_v<JSONCallbackObject, remove_cv_ref_t<Arg>> || is_derived_json_data_container_v<remove_cv_ref_t<Arg>>;

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList, typename... Args>
bool constexpr key_value_checker_v =
    arg_is_valid<Args...> || key_value_checker<CastableTypeList, TypeList, ArrayTypeList, Args...>::value;

template <typename... Args>
constexpr bool is_valid_args_v = key_value_checker_v<primitive_json_types, arguments_types, arguments_array_types, Args...>;

#else

template <typename... Args>
constexpr bool is_valid_args_v = true;

#endif
// ---------------------------------------------------------------------------
//  count_string_view_args_v<Args...>
//  Compte le nombre de std::string_view aux positions impaires de Args
//  (positions des valeurs dans les paires clé-valeur).
//  Utilisé pour dimensionner le string pool du StreamCursor à la construction.
// ---------------------------------------------------------------------------

template <typename... Args>
struct string_view_arg_counter { static constexpr size_t value = 0; };

template <typename Key, typename Value, typename... Rest>
struct string_view_arg_counter<Key, Value, Rest...> {
    static constexpr size_t value =
        (std::is_same_v<remove_cv_ref_t<Value>, std::string_view> ? 1 : 0) +
        string_view_arg_counter<Rest...>::value;
};

template <typename... Args>
constexpr size_t count_string_view_args_v = string_view_arg_counter<Args...>::value;

template<typename T>
constexpr bool is_basic_value = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
constexpr bool is_callback = std::is_same_v<JSONCallbackObject, remove_cv_ref_t<T>>;

template<typename T>
constexpr bool is_coords = container_info<T>::dimensions == 1 && container_info<T>::extent == 2 && std::is_floating_point_v<typename container_info<T>::base_t>;

template <typename T>
constexpr bool is_uint_array_v = container_info<T>::kind == ContainerKind::C_ARRAY &&
                                 container_info<T>::dimensions == 1 &&
                                 std::is_unsigned_v<typename container_info<T>::base_t>;
/*
  type_list utilities
*/

/*
  Build variant types from a type list
*/

template <typename TypeList> struct to_variant;

template <typename... Ts> struct to_variant<type_list<Ts...>> {
  using type = std::variant<Ts...>;
};


