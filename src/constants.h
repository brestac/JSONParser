#pragma once

#include <limits>
#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <type_traits>

#include "macros.h"

NAMESPACE_JSON_BEGIN

static size_t GLOBAL_PARSER_SIZE = 0;
static size_t MAX_GLOBAL_PARSER_SIZE = 0;
static size_t GLOBAL_STRING_POOL_SIZE = 0;
static const char *EMPTY_STRING = "";

constexpr size_t STREAM_BUFFER_SIZE = 1 << 7;     // 128 octets
constexpr size_t RING_BUFFER_SIZE = 1 << 8;       // 256 octets
constexpr size_t MAX_STRING_POOL_SIZE = 1 << 12;  // 4096 octets
constexpr size_t MAX_JSON_LENGTH = 1 << 24;       // 16777216 octets = 16MB
#ifdef ARDUINO
constexpr size_t MAX_STRING_POOL_REUSE_COUNT = 0;
constexpr size_t MAX_KEY_LENGTH   = 1 << 5;  // 32 octets
constexpr size_t MAX_VALUE_LENGTH = 1 << 6;  // 64 octets
#else
constexpr size_t MAX_STRING_POOL_REUSE_COUNT = 1 << 5; // 32 valeurs
constexpr size_t MAX_KEY_LENGTH   = 1 << 8;  // 256 octets
constexpr size_t MAX_VALUE_LENGTH = 1 << 8;  // 256 octets
#endif
constexpr size_t MAX_ARRAY_LENGTH = 1 << 16;      // 65536 valeurs
constexpr size_t MAX_KEY_VALUE_COUNT = 32;        // Maximum autorisé par la macro
constexpr size_t DEBUG_COLUMN_WIDTH = 80;

constexpr uint8_t VERSION = 1;
inline bool PRINT_BUFFER_AS_HEX = false;
inline size_t MAX_PRINTF_BUFFER_SIZE = 4096;
inline size_t MAX_POINTER_CURSOR_SIZE = std::numeric_limits<uint32_t>::max(); // Equals to 4294967295 bytes (4GB)
inline size_t MAX_ITERATIONS = 1 << 31;                                       // 2147483648 itérations maximum
inline size_t MAX_JSON_DEPTH = 1 << 8; // 256 niveaux de profondeur maximum

NAMESPACE_JSON_END

static char JSON_SPACE_CHARACTERS[4] = {' ', '\t', '\n', '\r'};
static char JSON_HEX_CHARACTERS[3][2] = {{'a', 'f'}, {'A', 'F'}, {'0', '9'}};
static char JSON_KEY_CHARACTERS[5][2] = {{'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}, {'$', '$'}};
static char JSON_DIGIT_CHARACTERS_RANGE[2] = {'0', '9'};

static constexpr char JSON_START_CHARACTER = '{';
static constexpr char JSON_END_CHARACTER = '}';
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

// ---------------------------------------------------------------------------
//  équivalent C++17 de std::remove_cvref_t
// ---------------------------------------------------------------------------
template <class T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename... Ts> struct type_list {};
// ---------------------------------------------------------------------------
//   NullType,InfinityType, NaNType
// ---------------------------------------------------------------------------
struct NullType {};
struct InfinityType {};
struct NaNType {};
struct UnknownValueType;
struct JSONCallbackObject;
class Stream;

using parsed_types = type_list<bool, int, float, double, std::string_view, NullType>;

using arguments_types = type_list<UnknownValueType, JSONCallbackObject>;
using arguments_array_types =
    type_list<int8_t, int16_t, int32_t, uint8_t, uint16_t, uint32_t, char, float, UnknownValueType>;
using arguments_array_array_types = type_list<char>;
