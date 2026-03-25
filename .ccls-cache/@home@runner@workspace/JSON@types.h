#pragma once

#ifdef __EXCEPTIONS
#include <stdexcept>
#include <typeinfo>
#endif

#include <array>
#include <cstring> // memcpy
#include <functional>
#include <stdint.h>
#include <string_view>
#include <type_traits>
#include <variant>

#include "constants.h"
#include "macros.h"
//#include "scanner.h"
#include "JSONCallbackObject.h"
#include "JSONData.h"
#include "JSONKey.h"
#include "ParseValueResult.h"
#include "StreamScanner.h"
#include "UnknownValueType.h"
#include "utils.h"

#ifdef ARDUINO
#include "StreamCursor.h"
#endif
// ---------------------------------------------------------------------------
//  équivalent C++17 de std::remove_cvref_t
// ---------------------------------------------------------------------------

// template <typename T, typename... Ts>
// constexpr bool is_any_of = (std::is_same<T, Ts>::value || ...);

// template <typename T, typename... Ts>
// constexpr bool
//     is_convertible_from_one_of = (std::is_convertible<Ts, T>::value || ...);

// template <typename T, typename U>
// constexpr bool assign_null =
//     std::is_same_v<U, std::nullptr_t> &&std::is_pointer<T>::value;

// ---------------------------------------------------------------------------
//   Type checker
// ---------------------------------------------------------------------------

template <class... Args> constexpr bool args_exist = (sizeof...(Args) > 0);

// template <class... Args> constexpr bool no_args = sizeof...(Args) == 0;

template <class... Args>
constexpr bool args_are_pairs = (sizeof...(Args) % 2) == 0;

template <class... Args>
constexpr bool args_are_valid = args_exist<Args...> &&args_are_pairs<Args...>;

// template <typename T>
// constexpr bool is_not_pointer = !std::is_pointer<T>::value;

// template <class T>
// constexpr bool is_char_value = std::is_same_v<base_array_type<T>, char>;

// template <class T>
// constexpr bool is_char_array_v = is_array_value<T> &&is_char_value<T>;

// template <class T>
// constexpr bool is_char_array_array_v = is_array_value<T>
//     &&is_array_value<base_array_type<T>> &&is_char_value<base_array_type<T>>;

// template <class T>
// struct is_vector : std::false_type {};

// template <class T>
// struct is_vector<std::vector<T>> : std::true_type {};

// template <class T>
// constexpr bool is_vector_v = is_vector<T>::value;

template <class T>
struct is_std_array : std::false_type {};

template <class T, size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

// template <class T>
// constexpr bool is_std_array_v = is_std_array<T>::value;

template <typename T, typename From, typename = void>
struct is_static_castable_from : std::false_type {};

template <typename T, typename From>
struct is_static_castable_from<
    T, From, std::void_t<decltype(static_cast<T>(std::declval<From>()))>>
    : std::true_type {};

template <typename T, typename TypeList> struct is_castable_from_any;

template <typename T, typename... Ts>
struct is_castable_from_any<T, type_list<Ts...>>
    : std::disjunction<is_static_castable_from<T, Ts>...> {};

template <typename T, typename TypeList>
struct is_in_type_list : std::false_type {};

template <typename T, typename... Ts>
struct is_in_type_list<T, type_list<Ts...>>
    : std::disjunction<std::is_same<T, Ts>...> {};

// Type de container
enum class ContainerKind { NOT_CONTAINER, C_ARRAY, CHAR_ARRAY, STD_ARRAY, STD_VECTOR };

template <typename T> struct container_info {
  using base_type = T;
  static constexpr size_t dimensions = 0;
  static constexpr ContainerKind kind = ContainerKind::NOT_CONTAINER;
  static constexpr bool is_container = false;
};

// C-array
template <typename T, size_t N> struct container_info<T[N]> {
  using base_type = typename container_info<T>::base_type;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::C_ARRAY;
  static constexpr size_t extent = N;
  static constexpr bool is_container = true;
};

// Char array
template <size_t N> struct container_info<char[N]> {
  using base_type = char;
  static constexpr size_t dimensions = 1;
  static constexpr ContainerKind kind = ContainerKind::CHAR_ARRAY;
  static constexpr size_t extent = N;
  static constexpr bool is_container = false;
};

// std::array
template <typename T, size_t N> struct container_info<std::array<T, N>> {
  using base_type = typename container_info<T>::base_type;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::STD_ARRAY;
  static constexpr size_t extent = N;
  static constexpr bool is_container = true;
};

// std::vector
template <typename T> struct container_info<std::vector<T>> {
  using base_type = typename container_info<T>::base_type;
  static constexpr size_t dimensions = container_info<T>::dimensions + 1;
  static constexpr ContainerKind kind = ContainerKind::STD_VECTOR;
  static constexpr size_t extent = MAX_ARRAY_LENGTH;
  static constexpr bool is_container = true;
};

// ==========================================
// Container from list checker
// ==========================================
template <typename T>
constexpr bool is_container_v = container_info<T>::is_container;

template <typename T, typename TypeList>
struct is_container_from_list : std::false_type {};

// C-array
template <typename T, size_t N, typename TypeList>
struct is_container_from_list<T[N], TypeList>
    : is_in_type_list<typename container_info<T[N]>::base_type, TypeList> {};

// std::array
template <typename T, size_t N, typename TypeList>
struct is_container_from_list<std::array<T, N>, TypeList>
    : is_in_type_list<typename container_info<std::array<T, N>>::base_type,
                      TypeList> {};

// std::vector
template <typename T, typename TypeList>
struct is_container_from_list<std::vector<T>, TypeList>
    : is_in_type_list<typename container_info<std::vector<T>>::base_type,
                      TypeList> {};

template <typename T, typename TypeList>
inline constexpr bool is_container_from_list_v =
    is_container_from_list<T, TypeList>::value;

// ==========================================
// char array, char array array
// ==========================================

template <typename T> struct is_char_array : std::false_type {};

template <typename T, size_t N>
struct is_char_array<T[N]> : std::is_same<T, char> {};

template <typename T>
inline constexpr bool is_char_array_v = is_char_array<T>::value;

template <typename T> struct is_char_array_array : std::false_type {};

template <typename T, size_t N, size_t M>
struct is_char_array_array<T[N][M]>
    : std::integral_constant<bool, std::is_same_v<T, char>>{};

template <typename T>
inline constexpr bool is_char_array_array_v = is_char_array_array<T>::value;

// ==========================================
// JSONData
// ==========================================
template <typename T>
struct is_derived_json_data : std::is_base_of<JSONData, remove_cvref_t<T>> {};

template <typename T>
struct is_derived_json_data<T *> : is_derived_json_data<T> {};

template <typename T>
inline constexpr bool is_derived_json_data_v = is_derived_json_data<T>::value;

template <typename T>
constexpr bool is_derived_json_data_container_v =
    container_info<T>::is_container
        &&is_derived_json_data<typename container_info<T>::base_type>::value;

// ==========================================
// Cursor
// ==========================================

template <typename T, typename = void>
struct is_cursor_reader : std::false_type {};

template <typename T, typename = void>
struct is_cursor_writer : std::false_type {};

template <typename Cursor>
struct is_cursor_reader<Cursor> : std::is_same<JSON::PointerCursorReader, remove_cvref_t<Cursor>> {};

template <>
struct is_cursor_reader<JSON::PointerCursorReader> : std::true_type {};

template <>
struct is_cursor_writer<JSON::PointerCursorWriter> : std::true_type {};

template <>
struct is_cursor_writer<JSON::PointerCursorPrinter> : std::true_type {};

#ifdef ARDUINO
#include "StreamCursor.h"
template <>
struct is_cursor_reader<JSON::StreamCursor> : std::true_type {};

template <>
struct is_cursor_writer<JSON::StreamCursor> : std::true_type {};

#endif

template<typename T>
inline constexpr bool is_cursor_reader_v = is_cursor_reader<T>::value;

template<typename T>
inline constexpr bool is_cursor_writer_v = is_cursor_writer<T>::value;

template<typename T>
inline constexpr bool is_cursor_v = is_cursor_reader_v<T> || is_cursor_writer_v<T>;
// template <typename T, typename = void>
// struct is_cursor_writer : std::false_type {};

// template <>
// struct is_cursor_writer<JSON::PointerCursorWriter> : std::true_type {};

// #ifdef ARDUINO
// template <>
// struct is_cursor_writer<JSON::StreamCursor> : std::true_type {};
// #else
// template <>
// struct is_cursor_writer<JSON::PointerCursorPrinter> : std::true_type {};
// #endif

// template<typename T>
// inline constexpr bool is_cursor_writer_v = is_cursor_writer<T>::value;

// ==========================================
// Key Value checker
// ==========================================
template <typename T, typename = void>
struct is_convertible_to_indexed_key : std::false_type {};

template <typename T>
struct is_convertible_to_indexed_key<
    T, std::void_t<decltype(JSONIndexedKey(std::declval<T>()))>>
    : std::true_type {};

// template <typename T>
// inline constexpr bool is_convertible_to_indexed_key_v =
//     is_convertible_to_indexed_key<T>::value;

// template <typename CastableTypeList, typename ArrayTypeList>
// struct key_value_checker<CastableTypeList, ArrayTypeList> : std::true_type
// {};

// template <typename CastableTypeList, typename ArrayTypeList, typename T>
// struct key_value_checker<CastableTypeList, ArrayTypeList, T> :
// std::false_type {
// };

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename Value>
struct value_checker
    : std::disjunction<
          is_castable_from_any<remove_cvref_t<Value>, CastableTypeList>,
          is_in_type_list<remove_cvref_t<Value>, TypeList>,
          is_container_from_list<remove_cvref_t<Value>, ArrayTypeList>,
          is_char_array<remove_cvref_t<Value>>,
          is_char_array_array<remove_cvref_t<Value>>,
          is_derived_json_data<remove_cvref_t<Value>>,
          std::is_pointer<remove_cvref_t<Value>>,
          std::integral_constant<bool, is_derived_json_data_container_v<remove_cvref_t<Value>>>> {};

template <typename Key>
struct key_checker : std::is_constructible<JSONKey, Key> {};

template <typename CastableTypeList, typename ArrayTypeList, typename... Args>
struct key_value_checker;

template <typename CastableTypeList, typename TypeList,
          /*typename ArrayTypeList,*/ typename... Args>
struct key_value_checker<CastableTypeList, TypeList,
                         /*ArrayTypeList,*/ type_list<Args...>>
    : std::true_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename T>
struct key_value_checker<CastableTypeList, TypeList,
                         ArrayTypeList /*, ArrayArrayTypeList*/, T>
    : std::false_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList/*,
          typename ArrayArrayTypeList*/>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList/*, ArrayArrayTypeList*/>
    : std::false_type {};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename Key, typename Value,
          class... Rest>
struct key_value_checker<CastableTypeList, TypeList, ArrayTypeList,
                         /*ArrayArrayTypeList,*/ Key, Value, Rest...>
    : std::conjunction<
          key_checker<Key>,
          value_checker<CastableTypeList, TypeList, ArrayTypeList,
                        /*ArrayArrayTypeList,*/ Value>,
          key_value_checker<CastableTypeList, TypeList,
                            ArrayTypeList /*, ArrayArrayTypeList*/, Rest...>> {
};

template <typename CastableTypeList, typename TypeList, typename ArrayTypeList,
          /*typename ArrayArrayTypeList,*/ typename... Args>
bool constexpr key_value_checker_v = args_are_pairs<Args...>
    &&key_value_checker<CastableTypeList, TypeList, ArrayTypeList /*,
ArrayArrayTypeList*/, Args...>::value;
