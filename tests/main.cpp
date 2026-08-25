
#define DEBUG_ESP_PORT Serial
#define JSON_DEBUG_LEVEL 0
#define DISABLE_ARGS_CHECK 1

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string_view>
#include <vector>
#include <variant>

#include "FileStream.h"
#include "HardwareSerial.h"
#include "StreamString.h"

#include <JSONParser.h>
#include <JSONPrinter.h>
#include "structs.h"
//#include "../examples/JSONParserTest/test.h"
#define GEOJSON_SMALL_FILE_PATH "./small.geojson"
#define GEOJSON_MEDIUM_FILE_PATH "./medium.geojson"
#define GEOJSON_BIG_FILE_PATH "./big.geojson"

std::string read_file_to_string( const char* filename ) {
  std::ifstream file( filename );
  if ( !file.is_open() ) { throw std::runtime_error( "Could not open file" ); }
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

int a = 1;
float b = 2.0f;

// I would like to build a constexpr mapping between the variable name and the variable itself
// I already have a macro that transfroms (a,b,c) into ( "a", a, "b", b, "c", c )
constexpr size_t hash_compile_time(std::string_view str) {
    size_t hash = 14695981039346656037ULL;
    for (char c : str) {
        hash ^= static_cast<size_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

template <typename T>
struct RefType {
    size_t id;
    T& ref;

    constexpr RefType(std::string_view name, T& r)
        : id(hash_compile_time(name)), ref(r)  {}

   // // // move operator
     constexpr RefType(RefType&& other) : id(other.id), ref(other.ref) {}
   //  // copy operator
     constexpr RefType(const RefType& other) : id(other.id), ref(other.ref) {}
    // move assignment operator
    constexpr RefType& operator=(RefType&& other) {
        id = other.id;
        ref = other.ref;
        return *this;
    }
};
// --- Vos types de données personnalisés ---

// 4. Définition du Variant contenant vos types spécialisés

// 5. Structure Wrapper pour le conteneur (Contient le Hash + le Variant)
// struct ctxp_dispatch_tableEntry {
//     size_t hash;
//     ElementVariant value;
// };

// #define TO_STR(a) #a
// #define _PAIR_(a) TO_STR(a), a

std::string_view name_1 = "roger";
std::string_view name_2 = "albert";
int age = 45U;
float height = 1.80f;

// template <typename T, std::size_t N>
// struct MonConteneurMacro {
//     T data[N];
//     constexpr std::size_t size() const { return N; }
// };
template <typename T, typename... Ts>
inline constexpr bool is_in_v = (std::is_same_v<T, Ts> || ...);

// Étape 2 : Ajouter conditionnellement le type T au début de la liste
template <typename T, typename... Ts>
using prepend_if_unique_t = std::conditional_t<
    is_in_v<T, Ts...>, 
    type_list<Ts...>,     // Si T est déjà présent, on garde la liste inchangée
    type_list<T, Ts...>   // Si T n'est pas présent, on l'ajoute devant
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
  using type = typename type_list_prepend<RefType<Value>, typename extract_value_types<Rest...>::type>::type;
};

template <typename... Args>
using extract_value_types_t = typename extract_value_types<Args...>::type;

template <typename... Args>
using extracted_value_types_with_monospace_t = typename type_list_prepend<std::monostate, extract_value_types_t<Args...>>::type;

template <typename... Args>
using pair_variant_t = typename to_variant<extracted_value_types_with_monospace_t<Args...>>::type;

// ---------------------------------------------------------------------------
//  Construit directement le std::variant<Value1, Value2, ...>
// ---------------------------------------------------------------------------

template <std::size_t PairIndex, typename Variant, typename Tuple>
constexpr Variant make_dispatch_entry(Tuple& args) {
    using value_type =
        remove_cv_ref_t<decltype(std::get<PairIndex * 2 + 1>(args))>;
    return Variant{RefType<value_type>{
        std::string_view(std::get<PairIndex * 2>(args)),
        std::get<PairIndex * 2 + 1>(args)}};
}

template <typename Variant, typename Tuple, std::size_t... PairIndex>
constexpr auto make_dispatch_table(
    Tuple& args, std::index_sequence<PairIndex...>) {
    return std::array<Variant, sizeof...(PairIndex)>{
        make_dispatch_entry<PairIndex, Variant>(args)...};
}

template<typename T>
inline void fill_dispatch_table(T&& dispatch_table, size_t) {
    // Base case: do nothing
}

template<typename T, typename V, typename... Args>
void fill_dispatch_table(T&& dispatch_table, size_t idx, const char* key, V& value, Args &&...args) {
      std::string_view key_sv(key);
      dispatch_table.at(idx) = RefType<V>{key_sv, value};
      //std::cout << "key: " << key << " value: " << value << " idx: " << idx << std::endl;
      fill_dispatch_table(dispatch_table, idx+1, std::forward<Args>(args)...);
}


// template<typename... Args>
// constexpr auto create_dispatch_table(Args &&...args) {
//     constexpr size_t container_size = sizeof...(Args) / 2;
//     using variant_t = pair_variant_t<remove_cv_ref_t<Args>...>;
//     std::array<variant_t, container_size> dispatch_table{};
//     size_t pair_index = 0;
//     fill_dispatch_table(dispatch_table, pair_index, std::forward<Args>(args)...);
//     return dispatch_table;
// };

template <typename... Args>
constexpr auto create_dispatch_table(Args&&... args) {
    constexpr std::size_t N = sizeof...(args) / 2;
    using variant_t = pair_variant_t<remove_cv_ref_t<Args>...>;
    auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
    return make_dispatch_table<variant_t>(
        args_tuple, std::make_index_sequence<N>{});
}

template <typename VariableType, typename Table>
void dispatch(Table& dispatch_table, std::string_view var_name) {
    using ref_type = RefType<VariableType>;
    size_t hash = hash_compile_time(var_name);

    auto it = std::find_if(dispatch_table.begin(), dispatch_table.end(), [&hash](auto entry) {
      return std::holds_alternative<ref_type>(entry) && std::get<ref_type>(entry).id == hash;
    });

    if (it != dispatch_table.end()) {
      std::cout << "Found: " << std::get<RefType<VariableType>>(*it).ref << std::endl;
    } else {
      std::cout << "Aucune variable trouvée." << std::endl;
    }

/*
    for (const auto& entry : dispatch_table) {
      // check if the hash matches the hash of the entry
      if ( std::holds_alternative<RefType<VariableType>>(entry) ) {
        std::cout << "string_view" << std::endl;
        if (std::get<RefType<VariableType>>(entry).id == hash_compile_time(var_name)) {
          VariableType& var = std::get<RefType<VariableType>>(entry).ref;
          std::cout << "Found: " << var << std::endl;
          var = "new_value";
          break;
        }
      }
    }
*/
}

#define CREATE_DISPATCH_TABLE( ... ) create_dispatch_table(MACRO(__VA_ARGS__))
//auto dispatch_table_object = CREATE_DISPATCH_TABLE(name_1, height, name_2);
constexpr auto dispatch_table_api = create_dispatch_table("name_1", name_1, "name_2", name_2, "age", age, "height", height);

using dispatch_table_variant = std::decay_t<decltype(dispatch_table_api[0])>;
static_assert(std::is_same_v<std::variant_alternative_t<3, dispatch_table_variant>,
                             RefType<float>>);


int main() {

     // dispatch<std::string_view>(dispatch_table_object, "name_2");
     // std::cout << name_2 << std::endl;

    dispatch<std::string_view>(dispatch_table_api, "name_2");
    std::cout << name_2 << std::endl;

    
  
//   FeatureCollection<3> fc;
//   File input = LittleFS.open( GEOJSON_MEDIUM_FILE_PATH, "r" );
//   // //std::string input = read_file_to_string( GEOJSON_SMALL_FILE_PATH );
//   JSON::ParseResult r = fc.fromJSON( input );
//   r.print();
//   // std::printf( "error: %d %s\n", r.error, errorToString( r.error ) );
  
// #ifdef JSON_DEBUG_MEM
//   std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
//   std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
// #endif

//   std::printf("TOTAL ITERATIONS: %lu\n", JSON::GLOBAL_ITERATIONS);
  return 0;
}
