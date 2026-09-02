#pragma once

#include <algorithm>
#include <array>
#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <tuple>
#include <variant>

#include "macros.h"
#include "utils.h"

//#define LOWER_ALPHA_MASK 0b11000000;

// constexpr uint64_t hash_compile_time(std::string_view str) {
//   uint64_t hash = 14695981039346656037ULL;
//   for (char c : str) {
//     hash ^= static_cast<uint64_t>(c);
//     hash *= 1099511628211ULL;
//   }
//   return hash;
// }

// void get_common_bits_equal_to_0_1(unsigned char*bytes, size_t len, uint8_t&
// common_0, uint8_t& common_1) {

//   for (size_t i = 0; i < len; i++) {
//     common_0 &= ~bytes[i];
//     common_1 &= bytes[i];
//   }

//   // Remove the range_mask from common_0 and common_1
//   common_0 &= ~(LOWER_ALPHA_MASK);
//   common_1 &= ~(LOWER_ALPHA_MASK);
// }

// bool is_for_sure_not_in_set(unsigned char byte, unsigned char*bytes, size_t
// len) {
//   uint8_t common_0 = 0XFF;
//   uint8_t common_1 = 0XFF;
//   get_common_bits_equal_to_0_1(bytes, len, common_0, common_1);

//   return ((byte & common_0) != 0) || ((byte & common_1) != common_1);
// }
// ---------------------------------------------------------------------------
//  Construit directement le std::variant<Value1, Value2, ...>
// ---------------------------------------------------------------------------
template <typename VariantType> struct Entry {
  uint32_t hash;
  VariantType variant;
  size_t key_index;
//  int mask_index; // -1 si pas de mask pour les clés non indexées

  using variant_type = VariantType;

  constexpr Entry(uint32_t h, VariantType v, size_t ki/*, int mi*/)
      : hash(h), variant(v), key_index(ki)/*, mask_index(mi)*/ {}

  constexpr Entry()
      : hash(0), variant(std::monostate()), key_index(0)/*, mask_index(-1)*/ {}

  // Methods used by std::sort

  //constexpr Entry(const Entry&) = default;
  //constexpr Entry(Entry&&) = default;
  //constexpr Entry& operator=(const Entry& other) = default;
  //constexpr Entry& operator=(Entry&& other) = default;

// Compare 2 Entry
constexpr bool operator==(const Entry& other) const { return hash == other.hash; }
constexpr bool operator!=(const Entry& other) const { return hash != other.hash; }
constexpr bool operator<(const Entry& other) const { return hash < other.hash; }
constexpr bool operator>(const Entry& other) const { return hash > other.hash; }

// Compare Entry and uint32_t hash
constexpr bool operator==(uint32_t other_hash) const { return hash == other_hash; }
constexpr bool operator!=(uint32_t other_hash) const { return hash != other_hash; }
constexpr bool operator<(uint32_t other_hash) const { return hash < other_hash; }
constexpr bool operator>(uint32_t other_hash) const { return hash > other_hash; }

};

template <typename T> struct ref_wrapper {
  T* ref;

  constexpr ref_wrapper(T& r) : ref(&r) {}
  constexpr ref_wrapper(const ref_wrapper&) = default;
  constexpr ref_wrapper(ref_wrapper&&) = default;
  constexpr ref_wrapper& operator=(const ref_wrapper&) = default;
  constexpr ref_wrapper& operator=(ref_wrapper&&) = default;

  // constexpr RefType(const RefType& other) : ref(other.ref) {}

  // constexpr operator T&() const {
  //     return ref;
  // }
  // constexpr T*_ref() { return ref; }
  constexpr T& get() { return *ref; }

  // constexpr RefType& operator=(const RefType& other) {
  //     if (this != &other) {
  //         ref = other.ref;
  //     }
  // }
};

template <typename VariantType, size_t N> struct DispatchInfo {
  using entry_type = Entry<VariantType>;

  std::array<entry_type, N> entries;
  //bool is_generic_keys;
  size_t sv_count;
  bool is_sorted;

  constexpr DispatchInfo(std::array<Entry<VariantType>, N> e, /*bool igk,*/ size_t svc, bool is)
      : entries(e)/*, is_generic_keys(igk)*/, sv_count(svc), is_sorted(is) {}
};

template <typename TableInfo>
constexpr auto find_entry(TableInfo& info, std::string_view var_name) {
  using EntryType = typename TableInfo::entry_type;

  if (var_name.empty()) {
    return EntryType();
  }

  uint32_t hash = hash32(var_name);

  if (info.is_sorted) {
    auto it = std::lower_bound(info.entries.begin(), info.entries.end(), hash);
    if (it != info.entries.end() && it->hash == hash) {
      return *it;
    }
  } else {
    auto it = std::find_if(info.entries.begin(), info.entries.end(), [hash](const EntryType& entry) { return entry.hash == hash; });
    if (it != info.entries.end()) {
      return *it;
    }
  }

  return EntryType();
}

template <typename T, typename... Ts>
inline constexpr bool is_in_v = (std::is_same_v<T, Ts> || ...);

// Étape 2 : Ajouter conditionnellement le type T au début de la liste
template <typename T, typename... Ts> using prepend_if_unique_t =
    std::conditional_t<
        is_in_v<T, Ts...>,
        type_list<Ts...>, // Si T est déjà présent, on garde la liste inchangée
        type_list<T, Ts...> // Si T n'est pas présent, on l'ajoute devant
        >;

template <typename T, typename List> struct type_list_prepend;

template <typename T, typename... Ts>
struct type_list_prepend<T, type_list<Ts...>> {
  using type = prepend_if_unique_t<T, Ts...>;
};

template <typename... Args> struct extract_value_types {
  using type = type_list<>; // cas de base : pack vide
};

template <typename Key, typename Value, typename... Rest>
struct extract_value_types<Key, Value, Rest...> {
  using type = typename type_list_prepend<
      ref_wrapper<Value>, typename extract_value_types<Rest...>::type>::type;
};

template <typename... Args> using extract_value_types_t =
    typename extract_value_types<Args...>::type;

template <typename... Args> using extracted_value_types_with_monospace_t =
    typename type_list_prepend<std::monostate,
                               extract_value_types_t<Args...>>::type;

template <typename... Args> using pair_variant_t =
    typename to_variant<extracted_value_types_with_monospace_t<Args...>>::type;

template <size_t PairIndex, typename Variant, typename Tuple>
constexpr auto make_dispatch_entry(/*bool& is_generic,*/ size_t& sv_count, Tuple& args) {
  using value_type =
      remove_cv_ref_t<decltype(std::get<PairIndex * 2 + 1>(args))>;

  if constexpr (std::is_same_v<value_type, std::string_view>) {
    sv_count++;
  }

  //auto key_index_pair = get_key_and_mask_index(std::get<PairIndex * 2>(args));

  // if (key_index_pair.second >= 0) {
  //   is_generic = false;
  // }

  return Entry<Variant>{
          hash32(std::get<PairIndex * 2>(args)),
          ref_wrapper<value_type>{std::get<PairIndex * 2 + 1>(args)},
          PairIndex/*,
          key_index_pair.second*/
        };
}

template <typename VariantType, typename Tuple, std::size_t... PairIndex>
constexpr auto make_dispatch_table(bool& is_sorted/*, bool& is_generic*/, size_t& sv_count,
                                   Tuple& args,
                                   std::index_sequence<PairIndex...>) {
  using EntryType = Entry<VariantType>;

  auto table = std::array<EntryType, sizeof...(PairIndex)>{
      make_dispatch_entry<PairIndex, VariantType>(/*is_generic,*/ sv_count,
                                                  args)...};
#if SUPPORTS_CONSTANT_EVALUATED
  if (!std::__is_constant_evaluated()) {
    std::sort(table.begin(), table.end(), [](const EntryType& a, const EntryType& b) { return a < b; });
    is_sorted = true;
  }
#endif
  return table;
}

template <typename... Args>
constexpr auto create_dispatch_table(Args&&... args) {
  static_assert(sizeof...(Args) > 0 && sizeof...(Args) % 2 == 0,
                "Number of arguments must be even");
  // uint64_t start = now();
  using variant_t = pair_variant_t<remove_cv_ref_t<Args>...>;
  constexpr std::size_t N = sizeof...(args) / 2;

  auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

  //bool is_generic = true;
  size_t sv_count = 0;
  bool is_sorted = false;
  auto entries = make_dispatch_table<variant_t>(is_sorted/*, is_generic*/, sv_count, args_tuple, std::make_index_sequence<N>{});

#if SUPPORTS_CONSTANT_EVALUATED
  if (std::__is_constant_evaluated()) {
    #pragma message "Dispatch table is evaluated at compile time"
  } else {
    JSON_DEBUG_WARNING("Dispatch table is evaluated at runtime\n");
  }
#endif
  return DispatchInfo<variant_t, N>(entries/*, is_generic*/, sv_count, is_sorted);
  // JSON::TIME_PROFILER+=(now()-start);
}

// Alternative dispatch table.
// find(key) -> entry (type_index, ptr) -> std::tuple<Types...> -> Type &ref = *(const_cast<Type*>(ptr))
// struct DispatchEntry {
//   size_t hash;
//   size_t type_index;
//   void *value;
//   //void (*callback)(std::any&);
//   // std::any value;
// };

// template <typename... Args>
// struct IndexedTypeList {
//   using tuple = std::tuple<Args...>;
// };

// template <size_t N, typename TypeList>
// using get_type_at_index = std::tuple_element_t<N, typename TypeList::tuple>;

// template <size_t I, typename TypeList>
// bool call_at(const DispatchEntry& entry) {
//     if (I != entry.type_index) return false;
//     using ValueType = get_type_at_index<I, TypeList>;
//     ValueType& value = (*static_cast<ValueType*>(entry.value));
//     value = 2;

//     std::cout << "Dispatch value " << value << " at index " << I << std::endl;
//     return true;
// }

// template <typename TypeList, size_t... I>
// void _dispatch_by_entry(const DispatchEntry& entry, std::index_sequence<I...>) {
//     (call_at<I, TypeList>(entry) || ...);
// }

// template <typename TypeList>
// void dispatch_by_entry(const DispatchEntry& entry) {
//   constexpr size_t N = std::tuple_size<typename TypeList::tuple>::value;
//   _dispatch_by_entry<TypeList>(entry, std::make_index_sequence<N>{});
// }


// template <typename T, typename Tuple>
// struct type_index;

// // Spécialisation pour trouver l'index
// template <typename T, typename... Types>
// struct type_index<T, IndexedTypeList<Types...>> {
//     static_assert((std::is_same_v<T, Types> + ...) == 1, "Le type doit apparaître exactement une fois dans le tuple.");

//     static constexpr std::size_t value = []() {
//         std::size_t index = 0;
//         bool found = false;
//         ((found ? 0 : (std::is_same_v<T, Types> ? (found = true, 0) : ++index)), ...);
//         return index;
//     }();
// };

// // Raccourci pour simplifier l'utilisation
// template <typename T, typename Tuple>
// inline constexpr std::size_t type_index_v = type_index<T, Tuple>::value;

// template <typename T, typename... Ts>
// inline constexpr bool exists_v = (std::is_same_v<T, Ts> || ...);

// template <bool Unique, typename T, typename... Ts>
// struct _append_type;

// template <bool Unique, typename T, typename... Ts>
// struct _append_type<Unique, IndexedTypeList<Ts...>, T> {
//   using type = std::conditional_t<
//     Unique && exists_v<T, Ts...>,
//     IndexedTypeList<Ts...>, // Si T est déjà présent, on garde la liste inchangée
//     IndexedTypeList<Ts..., T> // Si T n'est pas présent, on l'ajoute après
//   >;
// };

// template <typename TypeList, typename T>
// using append_type = _append_type<false, TypeList, T>;

// template <typename TypeList, typename T>
// using append_type_unique = _append_type<true, TypeList, T>;

// // Exemple d'utilisation :
// using new_list1 = append_type<IndexedTypeList<>, int>::type; // IndexedTypeList<int>
// using new_list2 = append_type<new_list1, float>::type;
// using new_list3 = append_type_unique<new_list2, float>::type;

// using empty_list = IndexedTypeList<>;


// int main() {  

  // std::cout << "tuple size is " << std::tuple_size<new_list3::tuple>::value << std::endl;
  // std::cout << "tuple type at 1 is " << typeid(get_type_at_index<1, new_list3>).name() << std::endl;
  // std::cout << "index for type float is " << type_index_v<float, new_list3> << std::endl;

  // // dispatch_by_index<new_list3>(0);
  // // dispatch_by_index<new_list3>(1);

  // static int a = 1;

  // // Add type to the list
  // using dispatch_types_list = append_type_unique<empty_list, int>::type;
  // // Create DispatchEntry
  // constexpr auto entry = DispatchEntry{ hash32("a"), type_index_v<int, dispatch_types_list>, &a };

  // std::cout << "entry.hash: " << entry.hash << std::endl;

  // // get variable back from the entry
  // dispatch_by_entry<dispatch_types_list>(entry);

  // std::cout << "a=" << a << std::endl;
//  }