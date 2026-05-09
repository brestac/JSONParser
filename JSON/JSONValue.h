#pragma once
#include <type_traits>
#include <variant>
#ifdef __EXCEPTIONS
#include <stdexcept>
#endif
#include "constants.h"
#include "demangled.h"
#include "utils.h"

// ---------------------------------------------------------------------------
//   JSONValue
// ---------------------------------------------------------------------------

template <class T>
using base_array_type = remove_cvref_t<std::remove_extent_t<remove_cvref_t<T>>>;

template <class T>
constexpr bool is_array_value = std::is_array_v<remove_cvref_t<T>>;

template <class T>
constexpr bool is_unsigned_value = std::is_unsigned_v<base_array_type<T>>;

template <class T>
constexpr bool is_uint_array_v = is_array_value<T> &&is_unsigned_value<T>;

template <typename TypeList> struct to_variant;

template <typename... Ts> struct to_variant<type_list<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename TypeList> class AnyValue {
  using to_variant_t = typename to_variant<TypeList>::type;

public:
  template <typename To> AnyValue(To &&value) : data(std::forward<To>(value)) {}

  template <typename To> operator To() const noexcept {

    return std::visit(
        [](auto &&arg) -> To {
          using From = decltype(arg);

          if constexpr (std::is_pointer_v<To>) {
            if constexpr (std::is_same_v<From, NullType>) {
              return nullptr;
            }
            else {
              return To();
            }
          } else if constexpr (std::is_same_v<From, To>) {
            return arg;
          } else if constexpr (std::is_convertible_v<From, To>) {
            return static_cast<To>(arg);
          } else {
#if __GXX_RTTI
            print_demangled_types("BAD CAST when assigning %s to %s\n", arg, To());
#endif
#if __EXCEPTIONS
            throw std::bad_cast();
#endif
          }
        },
        data);
  }

  template <typename T> bool is() const noexcept {
    return std::holds_alternative<T>(data);
  }

  template <typename T> T &get() noexcept { return std::get<T>(data); }

  template <typename T> const T &get() const noexcept {
    return std::get<T>(data);
  }

  // If data is a string_view, and value is an unsigned array, copy the hex
  // string to the array
  template <typename T> std::enable_if_t<is_uint_array_v<T>, void> copyTo(T &value) const noexcept {
      if (is<std::string_view>()) {
        copy_hex_be_to_h(value, get<std::string_view>().data(), get<std::string_view>().length());
      }
  }

private:
  template <typename T> friend class AnyValue;

  template <typename T> AnyValue(const AnyValue<T> &other) : data(other.data) {}

  template <typename T> AnyValue &operator=(const AnyValue<T> &other) noexcept;

public:
  to_variant_t data;
};

using JSONValue = AnyValue<parsed_types>;
