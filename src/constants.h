#pragma once

#include <limits>
#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <type_traits>

#include "macros.h"

// template <typename T, size_t N>
// constexpr T create_charset_mask(const char (&set)[N], const uint8_t offset) {
//     T mask = 0;

//     for (uint8_t i = 0; i < N; ++i) {
//         mask |= (1ULL << (static_cast<uint8_t>(set[i]) - offset));
//     }

//     return mask;
// }

NAMESPACE_JSON_BEGIN

// Runtime options
static inline bool PRINT_BUFFER_AS_HEX = false;
static inline uint64_t GLOBAL_ITERATIONS = 0;
// static inline uint64_t TIME_PROFILER = 0;

#ifdef JSON_DEBUG_MEM
static uint16_t GLOBAL_PARSER_SIZE = 0;
static uint16_t MAX_GLOBAL_PARSER_SIZE = 0;
static uint16_t GLOBAL_STRING_POOL_SIZE = 0;
static uint16_t GLOBAL_CONTEXT_STACK_SIZE = UINT16_MAX;
#endif

// Compile-time options
constexpr uint8_t VERSION = 1;
constexpr uint16_t YIELD_EVERY = 128;
constexpr uint16_t STREAM_WRITER_BUFFER_SIZE = 1 << 7;     // 128 octets
constexpr uint16_t RING_BUFFER_SIZE = 1 << 8;       // 256 octets
constexpr uint16_t MAX_STRING_POOL_SIZE = 1 << 12;  // 4096 octets
constexpr uint32_t MAX_JSON_LENGTH = 1 << 24;       // 16777216 octets = 16MB
constexpr uint16_t MAX_NUMERIC_LENGTH = 25 ; // Pour les entiers et les flottants int64_t et double 

#ifdef ARDUINO
constexpr uint16_t MAX_STRING_POOL_REUSE_COUNT = 0;
constexpr uint16_t MAX_KEY_LENGTH   = 1 << 5;  // 32 octets
constexpr uint16_t MAX_VALUE_LENGTH = 1 << 8;  // 256 octets
#else
constexpr uint16_t MAX_STRING_POOL_REUSE_COUNT = 0;
constexpr uint16_t MAX_KEY_LENGTH   = 1 << 8;  // 256 octets
constexpr uint16_t MAX_VALUE_LENGTH = 1 << 8;  // 256 octets
#endif

constexpr uint32_t MAX_ARRAY_LENGTH = 1 << 16;      // 65536 valeurs
constexpr uint8_t MAX_KEY_VALUE_COUNT = 32;        // Maximum autorisé par la macro

// Options
constexpr bool ALLOW_PARSING_INTEGER_AS_FLOAT = true;
constexpr bool ALLOW_INTEGER_OVERFLOW = true;
constexpr bool FROM_JSON_USES_UPDATES = true;

constexpr uint16_t MAX_PRINTF_BUFFER_SIZE = 4096;
constexpr uint64_t MAX_ITERATIONS = std::numeric_limits<uint64_t>::max();          // 4294967295 itérations maximum
constexpr uint16_t MAX_JSON_DEPTH = 1 << 8; // 256 niveaux de profondeur maximum

NAMESPACE_JSON_END

constexpr uint8_t SPACE_CHARACTERS_COMMON_LOW = 0b11010000; // ~(' ') & ~('\t') & ~('\n') & ~('\r');

static constexpr char JSON_HEX_CHARACTERS_RANGES[3][2] = {{'a', 'f'}, {'A', 'F'}, {'0', '9'}};
static constexpr char JSON_KEY_CHARACTERS_RANGES[5][2] = {{'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}, {'$', '$'} };
static constexpr char JSON_DIGIT_CHARACTERS_RANGES[1][2] = {{'0', '9'}};

static constexpr char JSON_OBJECT_START_CHARACTER = '{';
static constexpr char JSON_OBJECT_END_CHARACTER = '}';
static constexpr char JSON_ARRAY_START_CHARACTER = '[';
static constexpr char JSON_ARRAY_END_CHARACTER = ']';
static constexpr char JSON_COLON_CHARACTER = ':';
static constexpr char JSON_COMMA_CHARACTER = ',';
static constexpr char JSON_QUOTE_CHARACTER = '"';
static constexpr char JSON_ESCAPE_CHARACTER = '\\';
static constexpr char JSON_TRUE[4] = {'t', 'r', 'u', 'e'};
static constexpr char JSON_FALSE[5] = {'f', 'a', 'l', 's', 'e'};
static constexpr char JSON_NULL[4] = {'n', 'u', 'l', 'l'};
static constexpr char JSON_NAN[3] = {'N', 'a', 'N'};
static constexpr char JSON_INFINITY[8] = {'I', 'n', 'f', 'i', 'n', 'i', 't', 'y'};
//static constexpr char JSON_DELIMITERS[8] = {',',  '}',  ']',  ' ', '\t', '\n', '\r', '\0'};
// ---------------------------------------------------------------------------
//  équivalent C++17 de std::remove_cvref_t
// ---------------------------------------------------------------------------
#if __cplusplus <= 201703L
template <class T> using remove_cv_ref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#else
template <class T> using remove_cv_ref_t = std::remove_cvref_t<T>;
#endif

template <typename... Ts> struct type_list {};
// ---------------------------------------------------------------------------
//   NullType,InfinityType, NaNType
// ---------------------------------------------------------------------------
struct NullType {};
struct InfinityType {};
struct NaNType {};
struct JSONCallbackObject;
class Stream;

using primitive_json_types = type_list<bool, int, float, double, std::string_view, NullType>;
using arguments_types = type_list<JSONCallbackObject>;
using arguments_array_types = type_list<int8_t, int16_t, int32_t, uint8_t, uint16_t, uint32_t, char, float>;
using arguments_array_array_types = type_list<char>;

using MaskType = uint32_t;

