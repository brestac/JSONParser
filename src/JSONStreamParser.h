#pragma once

// JSONStreamParser.h
//
// Fournit JSONStreamParser<N> : version de JSONParser qui lit depuis
// un Stream Arduino via un ring buffer de taille N (puissance de 2).
//
// L'implémentation est partagée via JSONParserBase<Cursor>, templatisée
// uniquement sur le type de curseur. L'API publique est identique à
#include <limits>

#include "ParseDispatchTable.h"
#include "demangled.h"
#include "types.h"
#include "utils.h"

using namespace std;
using namespace JSON;

// ============================================================
//  JSONParserBase<Cursor>
//  Toute la logique du parser, paramétrée uniquement par le
//  type de curseur (PointerCursor ou StreamCursor<N>).
//  Ne pas utiliser directement — utiliser JSONParser ou
//  JSONStreamParser<N>.
// ============================================================
template <typename Cursor> class JSONParserBase {

  template <typename ParserT, typename TableT, typename TupleT>
  friend ParseValueResult lookup_impl(void *parser_ptr, void *table_ptr,
                                      void *refs_ptr,
                                      const std::string_view &parsed_key);

public:
  enum ParserState : uint8_t {
    IDLE = 0,
    KEY = 1,
    COLON = 2,
    VALUE = 3,
    COMMA = 4,
    END = 5,
    ERROR = 6,
    STOPPED = 7
  };

  enum ParserError : uint8_t {
    NO_ERROR = 0,
    NO_OBJECT_START = 1,
    INVALID_KEY = 2,
    NO_COLON = 3,
    INVALID_VALUE = 4,
    NO_COMMA = 5,
    INVALID_OBJECT = 6
  };

  // ── Constructeur PointerCursor ─
  explicit JSONParserBase(std::string_view &name,
                          const PointerCursorReader &cursor)
      : _cursor(cursor), _bytesConsumed(cursor.bytesConsumed()), _state(IDLE),
        _automask(false), _usemask(true), _keyMask(0), _nKeys(0), _nParsed(0),
        _nConverted(0), _nUpdated(0), _key_length(0),
        _is_top_level_array(false), _nArgs(0),
        _lastError(ParserError::NO_ERROR), _lastParseValueResult(0),
        _name(name) {
    JSON_DEBUG_COLOR(COLOR_BLUE, "JSONParserBase(pointer) '%.*s' created\n",
                     (int)_jsonParserName.length(), _jsonParserName.data());
  }

  // ── Constructeur StreamCursor ─────────────────────────────
  // Used when Cursor = StreamCursor; never called for other cursor types.
  explicit JSONParserBase(std::string_view &name, StreamCursor &cursor)
      : _cursor(cursor), _bytesConsumed(cursor.bytesConsumed()), _state(IDLE),
        _automask(false), _usemask(true), _keyMask(0), _nKeys(0), _nParsed(0),
        _nConverted(0), _nUpdated(0), _key_length(0),
        _is_top_level_array(false), _nArgs(0),
        _lastError(ParserError::NO_ERROR), _lastParseValueResult(0),
        _name(name) {
    JSON_DEBUG_COLOR(COLOR_BLUE, "JSONParserBase(stream) '%.*s' created\n",
                     (int)_jsonParserName.length(), _jsonParserName.data());
  }

  ~JSONParserBase() {
    // Do not destroy the Cursor because it may be used by a parent parser.
    JSON_DEBUG_WARNING("JSONParserBase '%.*s' ", (int)_name.length(),
                       _name.data());
    reset();
    JSON_DEBUG_WARNING("destroyed\n");
  }

  // ── API publique (identique à JSONParser) ─────────────────

  template <typename T>
  enable_if_t<is_derived_json_data_container_v<T>, void> parse(T &jsonObjects);

  template <typename... Args> void parse(Args &&...args);

  size_t parsed_length() { return _cursor.bytesConsumed() - _bytesConsumed; }
  ParserState state() { return _state; }
  uint8_t error() { return _lastError; }

  // ── Méthodes d'assignation (identiques à JSONParser) ──────
  // (reprises telles quelles — logique pure, pas d'accès au curseur)

  template <typename PV, typename V>
  ParseValueResult assign_integral_to_integral(PV &pv, V &v);

  template <typename PV, typename V>
  ParseValueResult assign_same_type(PV &pv, V &v);

  template <typename PV, typename V>
  ParseValueResult assign_convertible(PV &pv, V &v);

  template <typename PV, typename V>
  ParseValueResult assign_string_view_to_char_array(PV &pv, V &v);

  template <typename PV, typename V>
  ParseValueResult assign_null_ptr_to_pointer(PV &pv, V &v);

  template <typename V> ParseValueResult assign_array_to_array(V &pv, V &v);

  template <typename PV, typename V>
  ParseValueResult assign_not_handled(PV &pv, V &v);

  template <typename PV, typename V>
  ParseValueResult assign_parsed_value_to_value(PV &pv, V &v);

  template <typename V>
  ParseValueResult assign_string_view_to_unsigned_array(std::string_view pv,
                                                        V &v);

  template <typename PV>
  ParseValueResult assign_callback_object(const PV &pv, JSONCallbackObject &cb);

  template <typename PV, typename V>
  ParseValueResult assign_infinity_to_integral(PV &pv, V &v);

  template <class From, class To> constexpr To clamp_to_max(From v);

  template <typename V> ParseValueResult parse_into_value(V &arg_value);

  ParseValueResult parse_into_array_at_index(JSONCallbackObject &cb,
                                             uint32_t index);

  template <typename T, size_t N2>
  ParseValueResult parse_into_array_at_index(T (&array)[N2], uint32_t index);

  template <typename T>
  ParseValueResult parse_into_array_at_index(std::vector<T> &array,
                                             uint32_t index);

  template <typename T, size_t N2>
  ParseValueResult parse_into_array_at_index(std::array<T, N2> &array,
                                             uint32_t index);

  template <typename V>
  enable_if_t<container_info<V>::is_container ||
                  std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>,
              ParseValueResult>
  parse_array(V &arg_value);

  // Accessors
  size_t nParsed() { return _nParsed; }
  size_t nConverted() { return _nConverted; }
  size_t nUpdated() { return _nUpdated; }
  size_t nKeys() { return _nKeys; }
  uint32_t keyMask() { return _keyMask; }
  bool automask() { return _automask; }
  void setAutomask(bool automask) { _automask = automask; }
  void setUseMask(bool useMask) { _usemask = useMask; }
  bool stopped() { return _state == STOPPED; }
  void setName(std::string_view name) { _name = name; }
  std::string_view name() { return _name; }
  void reset_string_pool() { s_pool_offset = 0; }
  // Alloue ou agrandit le pool si pool_limit > taille actuelle.
  // Appelé depuis le constructeur avant tout parsing.
  static void set_pool_size(size_t pool_limit) {
    if (pool_limit == 0) return;
    if (pool_limit <= s_pool_size) return;  // déjà suffisant
    char* p = static_cast<char*>(realloc(s_string_pool, pool_limit));
    if (p) {
      s_string_pool = p;
      s_pool_size   = pool_limit;
      JSON_DEBUG_INFO("StreamCursor: pool agrandi à %zu octets\n", pool_limit);
    } else {
      JSON_DEBUG_WARNING("StreamCursor: realloc échoué pour %zu octets\n", pool_limit);
    }
  }

private:
  Cursor &_cursor; // ← reference: shared across nested parsers
  size_t _bytesConsumed;
  ParserState _state;
  bool _automask;
  bool _usemask;
  uint32_t _keyMask;
  size_t _nKeys;
  size_t _nParsed;
  size_t _nConverted;
  size_t _nUpdated;
  size_t _key_length;
  char _key_buf[JSON::MAX_KEY_LENGTH + 1];
  char _val_buf[JSON::MAX_VALUE_LENGTH + 1];
  bool _is_top_level_array;
  uint8_t _nArgs;
  ParserError _lastError;
  ParseValueResult _lastParseValueResult;
  std::string_view _name;
  static char*  s_string_pool;  // pointeur heap, nullptr jusqu'au premier appel
  static size_t s_pool_size;    // taille actuellement allouée
  static size_t s_pool_offset;  // offset courant dans le pool
  void reset();

  // ── Primitives de lecture via curseur ──────────────────────
  // Ces méthodes encapsulent tous les accès au curseur.
  // Elles appellent cursor_scan_*() ou les équivalents PointerCursor.

  bool _peek_char(char c) {
    return cursor_scan_char(_cursor, c, /*include=*/false);
  }

  bool _consume_char(char c) {
    return cursor_scan_char(_cursor, c, /*include=*/true);
  }

  char _current_char() {
    int c = _cursor.peek();
    return c < 0 ? '\0' : static_cast<char>(c);
  }

  // ── Méthodes de parsing (logique identique à JSONParser) ───

  bool parse_key();

  ParseValueResult parse_value(JSONCallbackObject &cb);
  ParseValueResult parse_value(UnknownValueType &unknown);

  template <typename TupleT, typename TableT>
  std::enable_if_t<(std::tuple_size<TupleT>::value == 1), ParseValueResult>
  parse_value(TableT &table, TupleT &args);

  template <typename TupleT, typename TableT>
  std::enable_if_t<(std::tuple_size<TupleT>::value > 1), ParseValueResult>
  parse_value(TableT &table, TupleT &args);

  void _reset_key();
  bool scan_escaped_string(std::string_view &sv);
  template <typename V> ParseValueResult parse_string(V &v);
  template <typename V, typename Type> ParseValueResult parse_numeric(V &v);
  template <typename V> ParseValueResult parse_floating_point(V &v);
  template <typename V> ParseValueResult parse_integer(V &v);
  template <typename V> ParseValueResult parse_numeric(V &v);
  template <typename V> ParseValueResult parse_bool(V &v);
  template <typename V> ParseValueResult parse_null(V &v);
  template <typename V> ParseValueResult parse_nan(V &v);
  template <typename V> ParseValueResult parse_infinity(V &v);
  ParseValueResult parse_array(UnknownValueType);

  template <typename V> ParseValueResult parse_object(V &v);
  template <typename V> ParseValueResult parse_any(V v);

  ParseValueResult parse_unknown_value();

  size_t bytesConsumed() { return _cursor.bytesConsumed(); }
  bool parse_colon();
  bool parse_comma();
  bool is_object_start() { return _current_char() == JSON_START_CHARACTER; }
  bool is_object_end() { return _current_char() == JSON_END_CHARACTER; }
  bool is_array_start() {
    return _current_char() == JSON_ARRAY_START_CHARACTER;
  }
  bool is_array_end() { return _current_char() == JSON_ARRAY_END_CHARACTER; }
  bool skip_spaces() { return cursor_skip_spaces(_cursor); }
  size_t scan_digits(size_t max_length = 0);

  void set_state(ParserState s);
  void print_state(size_t iteration);
  std::string_view get_state_name();
  const char *errorToString(ParserError error);
  const char *parsedValueTypeToString(ParseValueResult error);
};

// Définitions des membres statiques (C++17 inline)
template <typename Cursor> inline char* JSONParserBase<Cursor>::s_string_pool = nullptr;
template <typename Cursor> inline size_t JSONParserBase<Cursor>::s_pool_size   = 0;
template <typename Cursor> inline size_t JSONParserBase<Cursor>::s_pool_offset = 0;

// ============================================================
//  Implémentation des méthodes
// ============================================================

template <typename Cursor> void JSONParserBase<Cursor>::reset() {
  // _cursor is passed from parser to parser and should not be reset
  //_bytesConsumed = 0;
  _automask = false;
  _usemask = true;
  _keyMask = 0;
  _nKeys = 0;
  _nParsed = 0;
  _nConverted = 0;
  _nUpdated = 0;
  _state = IDLE;
  _key_length = 0;
  //_key_buf[0] = '\0';
  //_val_buf[0] = '\0';
  _is_top_level_array = false;
  _nArgs = 0;
  _lastError = ParserError::NO_ERROR;
  _lastParseValueResult = 0;
  _name = "$ROOT";
}

template <typename Cursor>
void JSONParserBase<Cursor>::set_state(ParserState s) {
  if (_state == END || _state == ERROR || _state == STOPPED)
    return;
  _state = s;
  if (_state == COMMA) {
      _reset_key();
  }
}

template <typename Cursor> void JSONParserBase<Cursor>::_reset_key() {
  _key_buf[0] = '\0';
  _key_length = 0;
}

// ── parse_key ────────────────────────────────────────────────
template <typename Cursor> bool JSONParserBase<Cursor>::parse_key() {
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true)) {
    _reset_key();
    return false;
  }

  // Pour StreamCursor on doit copier la clé dans un buffer local.
  // Pour PointerCursor on peut pointer directement (comportement original).
  // On utilise un buffer statique court pour la clé.
  size_t n = 0;

  while (n < JSON::MAX_KEY_LENGTH) {
    int c = _cursor.peek(n);
    if (c < 0) {
      _reset_key();
      return false;
    }
    char ch = static_cast<char>(c);
    // Valide si dans JSON_KEY_CHARACTERS (ranges a–z A–Z 0–9 _ $)
    bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                 (ch >= '0' && ch <= '9') || ch == '_' || ch == '$';
    if (!valid)
      break;
    _key_buf[n++] = ch;
  }
  
  if (n == 0) {
    return false;
  }

  _cursor.advance(n); // consomme les caractères de la clé

  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true)) {
    _reset_key();
    return false;
  }

  _key_length = n;

  JSON_DEBUG_INFO("JSONParserBase::parse_key '%.*s'\n", (int)_key_length, _key_buf);
  return true;
}

// ── parse_colon ───────────────────────────────────────────────
template <typename Cursor> bool JSONParserBase<Cursor>::parse_colon() {
  cursor_skip_spaces(_cursor);
  return cursor_scan_char(_cursor, JSON_COLON_CHARACTER, true);
}

// ── parse_comma ───────────────────────────────────────────────
template <typename Cursor> bool JSONParserBase<Cursor>::parse_comma() {
  return cursor_scan_char(_cursor, JSON_COMMA_CHARACTER, true);
}

// ── scan_digits ───────────────────────────────────────────────
template <typename Cursor>
size_t JSONParserBase<Cursor>::scan_digits(size_t max_length) {
  size_t n = 0;
  while (max_length == 0 || n < max_length) {
    int c = _cursor.peek(n);
    if (c < 0)
      break;
    char ch = static_cast<char>(c);
    if (ch < '0' || ch > '9')
      break;
    n++;
  }
  _cursor.advance(n);
  return n;
}

// Get a string_view from the cursor, handling multiple escape sequences
// We write from the cursor to the static string pool, and build a string_view from it.
// The resulting string_view is passed as reference to the caller.
template <typename Cursor>
bool JSONParserBase<Cursor>::scan_escaped_string(std::string_view &sv) {
  bool escaped = false;
  size_t n = 0;
  char *pool_start = s_string_pool + s_pool_offset;
  
  while (n < JSON::MAX_VALUE_LENGTH) {
    int c = _cursor.peek(n);
    if (c < 0)
      break;
    char ch = static_cast<char>(c);
    if (ch == JSON_ESCAPE_CHARACTER) {
      escaped = true;
      n++;
      continue;
    }
    if (ch == JSON_QUOTE_CHARACTER && !escaped) {
      break;
    }
    pool_start[n] = ch;
    n++;
  }

  if (n == 0)
    return false;

  sv = std::string_view(pool_start, n);
  s_pool_offset += n;
  _cursor.advance(n);

  return true;
}

// For the PointerCursorReader, we scan the string as we would normally do for non escaped strings.
// Then we peek back by one and check if the previous character was an escape character.
// If it was, we go back to the start of the string and scan it again, this time handling escape sequences.
// Then we advance the cursor by the length of the string.
template <>
template <typename V>
ParseValueResult JSONParserBase<PointerCursorReader>::parse_string(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_string\n");
  const char * start = _cursor.ptr();
  
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  if (!cursor_scan_until(_cursor, JSON_QUOTE_CHARACTER, MAX_VALUE_LENGTH, true, false)) {
    return ParseValueResult::NO_RESULT;
  }

  std::string_view parsed_value(start, _cursor.ptr() - start);

  if (_cursor.peek(-1) == JSON_ESCAPE_CHARACTER) {
    _cursor.advance_to(start);
    if (!scan_escaped_string(parsed_value)) {
      return ParseValueResult::NO_RESULT;
    }
    _cursor.advance();
  }

  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  return ParseValueResult::VALUE_PARSED | assign_parsed_value_to_value(parsed_value, arg_value);
}

// For the streamCursor, the static string pool is mandatory to store the string.
template <>
template <typename V>
ParseValueResult JSONParserBase<StreamCursor>::parse_string(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_string\n");
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  std::string_view parsed_value;
  if (!scan_escaped_string(parsed_value)) {
    return ParseValueResult::NO_RESULT;
  }

  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  return ParseValueResult::VALUE_PARSED | assign_parsed_value_to_value(parsed_value, arg_value);
}

// ── parse_integer ────────────────────────────────────────────
// Extrait les digits dans un buffer local, puis appelle strtol.
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_integer(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_integer\n");
  if constexpr (std::is_integral_v<V> && sizeof(V) > 4) {
    if constexpr (std::is_unsigned_v<V>)
      return parse_numeric<V, uint64_t>(arg_value);
    else
      return parse_numeric<V, int64_t>(arg_value);
  } else if constexpr (std::is_integral_v<V> && std::is_unsigned_v<V> &&
                       sizeof(V) == 4) {
    return parse_numeric<V, uint64_t>(arg_value);
  } else {
    return parse_numeric<V, int32_t>(arg_value);
  }
}

// ── parse_floating_point ──────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_floating_point(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_floating_point\n");
  return parse_numeric<V, double>(arg_value);
}

template <typename Cursor>
template <typename V, typename Type>
ParseValueResult JSONParserBase<Cursor>::parse_numeric(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_numeric\n");

  ParseValueResult result = ParseValueResult::NO_RESULT;

  char *start;

  if constexpr (std::is_same_v<Cursor, const PointerCursorReader>) {
    start = const_cast<char *>(_cursor.ptr());
  } else {
    static char tmp[64];
    size_t len = _cursor.peekToken(tmp, sizeof(tmp) - 1);
    if (len == 0)
      return result;

    tmp[len] = '\0';
    start = tmp;
  }

  Type parsed_value;
  char *end;

  if constexpr (std::is_same_v<Type, double>) {
    parsed_value = std::strtod(start, &end);
    JSON_DEBUG_INFO("JSONParserBase::parse_numeric double %f\n", parsed_value);
  } else if constexpr (std::is_same_v<Type, int32_t>) {
    parsed_value = (int)std::strtol(start, &end, 10);
    JSON_DEBUG_INFO("JSONParserBase::parse_numeric integer %d\n", parsed_value);
  } else if constexpr (std::is_same_v<Type, int64_t>) {
    parsed_value = std::strtoll(start, &end, 10);
    JSON_DEBUG_INFO("JSONParserBase::parse_numeric int64 %lld\n",
                    (long long)parsed_value);
  } else if constexpr (std::is_same_v<Type, uint64_t>) {
    parsed_value = std::strtoull(start, &end, 10);
    JSON_DEBUG_INFO("JSONParserBase::parse_numeric uint64 %llu\n",
                    (unsigned long long)parsed_value);
  }

  // check if parsed_value have been parsed as Infinity
  if (parsed_value == std::numeric_limits<Type>::infinity()) {
    return parse_infinity(arg_value);
  }

  size_t consumed = static_cast<size_t>(end - start);

  if (consumed == 0)
    return result;

  _cursor.advance(consumed);

  if constexpr (std::is_integral_v<Type>) {
    if (cursor_scan_char(_cursor, '.', true)) {
      JSON_DEBUG_WARNING("JSONParserBase::parse_numeric integer: found extra "
                         "digits after '.'\n");
      scan_digits(JSON::MAX_VALUE_LENGTH);
    }
  }

  return ParseValueResult::VALUE_PARSED |
         assign_parsed_value_to_value(parsed_value, arg_value);
}

// ── parse_bool ───────────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_bool(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_bool\n");
  if (cursor_scan_keyword(_cursor, JSON_FALSE, true)) {
    bool pv = false;
    return ParseValueResult::VALUE_PARSED |
           assign_parsed_value_to_value(pv, arg_value);
  }
  if (cursor_scan_keyword(_cursor, JSON_TRUE, true)) {
    bool pv = true;
    return ParseValueResult::VALUE_PARSED |
           assign_parsed_value_to_value(pv, arg_value);
  }
  return ParseValueResult::NO_RESULT;
}

// ── parse_null ───────────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_null(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_null\n");
  if (!cursor_scan_keyword(_cursor, JSON_NULL, true))
    return ParseValueResult::NO_RESULT;
  NullType pv;
  return ParseValueResult::VALUE_PARSED |
         assign_parsed_value_to_value(pv, arg_value);
}

// ── parse_nan ────────────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_nan(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_nan\n");
  if (!cursor_scan_keyword(_cursor, JSON_NAN, true))
    return ParseValueResult::NO_RESULT;
  NullType pv;
  return ParseValueResult::VALUE_PARSED |
         assign_parsed_value_to_value(pv, arg_value);
}

// ── parse_infinity ───────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_infinity(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_infinity\n");
  if (!cursor_scan_keyword(_cursor, JSON_INFINITY, true))
    return ParseValueResult::NO_RESULT;
  InfinityType pv;
  return ParseValueResult::VALUE_PARSED |
         assign_parsed_value_to_value(pv, arg_value);
}

// ── parse_numeric ────────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_numeric(V &arg_value) {
  if constexpr (std::is_same_v<V, JSONCallbackObject> ||
                std::is_same_v<V, UnknownValueType>) {
    bool ok = parse_floating_point(arg_value).parsed() ||
              parse_integer(arg_value).parsed() ||
              parse_nan(arg_value).parsed() ||
              parse_infinity(arg_value).parsed();
    return ok ? ParseValueResult::VALUE_PARSED : ParseValueResult::NO_RESULT;
  } else if constexpr (std::is_floating_point_v<remove_cvref_t<V>>) {
    return parse_floating_point(arg_value);
  } else if constexpr (std::is_integral_v<remove_cvref_t<V>> &&
                       !std::is_same_v<remove_cvref_t<V>, bool>) {
    return parse_integer(arg_value);
  }
  return ParseValueResult::NO_RESULT;
}

// ── parse_unknown_value ───────────────────────────────────────
// Saute une valeur JSON sans la parser (objet, tableau, littéral...)
template <typename Cursor>
ParseValueResult JSONParserBase<Cursor>::parse_unknown_value() {
  JSON_DEBUG_INFO("JSONParserBase::parse_unknown_value\n");
  [[maybe_unused]] size_t iterations = 0;
  int depth = 0;
  bool inString = false;
  bool escape = false;

  while (true) {
    iterations++;
    int c = _cursor.peek();

    if (c < 0)
      return ParseValueResult::NO_RESULT;

    char ch = static_cast<char>(c);

    if (escape) {
      escape = false;
    } else if (ch == JSON_ESCAPE_CHARACTER && inString) {
      escape = true;
    } else if (ch == JSON_QUOTE_CHARACTER) {
      inString = !inString;
    }

    if (inString) {
      _cursor.advance();
      continue;
    }

    if (ch == JSON_START_CHARACTER || ch == JSON_ARRAY_START_CHARACTER) {
      depth++;
    } else if (ch == JSON_END_CHARACTER || ch == JSON_ARRAY_END_CHARACTER) {
      if (depth == 0) {
        break;
      }
      depth--;
    } else if (ch == JSON_COMMA_CHARACTER && depth == 0) {
      break;
    }

    if (depth < 0)
      break;

    _cursor.advance();
  }

  JSON_DEBUG_INFO("JSONParserBase::parse_unknown_value iterations=%zu\n",
                  iterations);

  return ParseValueResult::VALUE_PARSED;
}

// ── parse_value (callback) ────────────────────────────────────
template <typename Cursor>
ParseValueResult JSONParserBase<Cursor>::parse_value(JSONCallbackObject &cb) {
  JSON_DEBUG_WARNING("JSONParserBase<Cursor>::parse_value with callback\n");

  cb.setKey(_key_buf, _key_length);

  if (_is_top_level_array) {
    JSON_DEBUG_INFO("JSONParserBase<Cursor>::parse_value top level array\n");
    return ParseValueResult::KEY_FOUND | parse_array(cb);
  }

  return ParseValueResult::KEY_FOUND | parse_into_value(cb);
}

template <typename Cursor>
ParseValueResult
JSONParserBase<Cursor>::parse_value(UnknownValueType &unknown) {
  JSON_DEBUG_WARNING("JSONParserBase<Cursor>::parse_value UnknownValueType\n");

  return ParseValueResult::KEY_FOUND | parse_into_value(unknown);
}

template <typename Cursor>
template <typename TupleT, typename TableT>
std::enable_if_t<(std::tuple_size<TupleT>::value == 1), ParseValueResult>
JSONParserBase<Cursor>::parse_value(TableT &table, TupleT &args) {
  return parse_value(std::get<0>(args));
}

template <typename Cursor>
template <typename TupleT, typename TableT>
std::enable_if_t<(std::tuple_size<TupleT>::value > 1), ParseValueResult>
JSONParserBase<Cursor>::parse_value(TableT &table, TupleT &args) {
  constexpr size_t NPairs = std::tuple_size<TupleT>::value / 2;
  const std::string_view parsed_key(_key_buf, _key_length);

  const StaticEntry *entry = table.find(hash32(parsed_key));

  if (!entry) {
    JSON_DEBUG_WARNING(
        "JSONParserBase<Cursor>::parse_value key '%.*s' not found\n",
        (int)parsed_key.length(), parsed_key.data());
    return ParseValueResult::NO_RESULT;
  }

  ParseValueResult result = ParseValueResult(ParseValueResult::KEY_FOUND);
  result |= dispatch_by_index(entry->arg_index, *this, args,
                              std::make_index_sequence<NPairs>{});

  if (result.updated()) {
    if (_usemask) {
      const size_t mask_idx =
          _automask ? entry->arg_index : static_cast<size_t>(entry->key_index);
      _keyMask |= (1u << mask_idx);
    }
    _nUpdated++;
  }

  if (result.converted())
    _nConverted++;

  return result;
}

// ── parse_into_value ─────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_into_value(V &arg_value) {
  JSON_DEBUG_TYPES("JSONParserBase<Cursor>::parse_into_value %s\n", arg_value);
  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_same_v<V, bool>) {
    return parse_bool(arg_value) | ParseValueResult::BOOLEAN;
  } else if constexpr (std::is_floating_point_v<V>) {
    return parse_floating_point(arg_value) | ParseValueResult::FLOAT;
  } else if constexpr (std::is_integral_v<V>) {
    return parse_integer(arg_value) | ParseValueResult::INTEGER;
  } else if constexpr (std::is_same_v<V, std::string_view> ||
                       is_char_array_v<V>) {
    return parse_string(arg_value) | ParseValueResult::STRING;
  } else if constexpr (is_uint_array_v<V>) {
    ParseValueResult result = parse_string(arg_value);

    if (result.parsed()) {
      return result | ParseValueResult::STRING;
    }

    result = parse_array(arg_value);
    return result.parsed() ? result | ParseValueResult::ARRAY
                           : ParseValueResult::NO_RESULT;
  } else if constexpr (is_container_v<V>) {
    return parse_array(arg_value) | ParseValueResult::ARRAY;
  } else if constexpr (std::is_same_v<remove_cvref_t<V>, UnknownValueType>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_base_of_v<JSONObject, remove_cvref_t<V>>) {
    return parse_object(arg_value) | ParseValueResult::OBJECT;
  } else if constexpr (std::is_pointer_v<V>) {
    ParseValueResult result = parse_null(arg_value);
    if constexpr (!std::is_const_v<std::remove_pointer_t<V>> &&
                  !std::is_same_v<V, UnknownValueType>) {
      if (!result.parsed() && arg_value != nullptr)
        result |= parse_into_value(*arg_value) |
                  ParseValueResult::POINTER; // TODO: check if this
                                             // is correct
    }
    return result;
  } else {
    return ParseValueResult::NO_RESULT | ParseValueResult::UNKNOWN;
  }
}

template <typename Cursor>
template <typename T>
enable_if_t<is_derived_json_data_container_v<T>, void>
JSONParserBase<Cursor>::parse(T &jsonObjects) {
  JSON_DEBUG_INFO("JSONParserBase::parse with derived JSONObject objects\n");
  _is_top_level_array = true;
  parse_array(jsonObjects);
  _state = END;
}

// ── parse (boucle principale) ─────────────────────────────────
template <typename Cursor>
template <typename... Args>
void JSONParserBase<Cursor>::parse(Args &&...args) {
  static_assert(sizeof...(Args) > 0, "::parse No arguments provided");
  using TupleT = std::tuple<Args &&...>;
  constexpr size_t NPairs = sizeof...(Args) / 2;

  // Références runtime — reconstruites à chaque appel
  TupleT refs(std::forward<Args>(args)...);

  // Table statique — uniquement hash + index, pas de références
  // static constexpr possible car ne dépend QUE des const char[N]
  // qui sont des littéraux, stables pour toute la spécialisation
  static const StaticDispatchTable<NPairs> table(refs);

  _nArgs = sizeof...(Args);
  size_t iteration = 0;

  while (!_cursor.eof() && iteration <= JSON::MAX_ITERATIONS) {
    iteration++;
#if JSON_DEBUG_LEVEL > 0
    print_state(iteration);
#endif
    switch (_state) {
    case IDLE:
      skip_spaces();

      if (is_array_start()) {
        _is_top_level_array = true;
        _state = VALUE;
        continue;
      }

      if (is_object_start()) {
        _cursor.advance();
        _state = KEY;
      } else {
        _state = ERROR;
        _lastError = ParserError::NO_OBJECT_START;
      }
      break;

    case KEY:
      skip_spaces();
      if (is_object_end()) {
        _state = END;
        continue;
      } else if (parse_key()) {
        set_state(COLON);
      } else {
        _state = ERROR;
        _lastError = ParserError::INVALID_KEY;
      }
      break;

    case COLON:
      if (parse_colon()) {
        set_state(VALUE);
      } else {
        _state = ERROR;
        _lastError = ParserError::NO_COLON;
      }
      break;

    case VALUE: {
      skip_spaces();
      if (is_object_end()) {
        _state = END;
        continue;
      }

      ParseValueResult r = parse_value(table, refs);

      if (!r.keyFound()) { // The key was not found in the arguments. This is
                           // not an error.
        parse_unknown_value();
        set_state(COMMA);
      } else if (r.parsed()) {
        _nParsed++;
        set_state(COMMA);
      } else { // The key was found but the value was not parsed. This is an
               // error.
        _state = ERROR;
        _lastError = ParserError::INVALID_VALUE;
        _lastParseValueResult = r.valueType();
      }
      break;
    }

    case COMMA:
      skip_spaces();
      if (is_object_end()) {
        _state = END;
        break;
      }
      if (parse_comma()) {
        set_state(KEY);
      } else {
        _state = ERROR;
        _lastError = ParserError::NO_COMMA;
      }
      break;

    case END:
      _cursor.advance();
      JSON_DEBUG_INFO(
          "JSONParserBase: parsing complete, iterations=%zu position=%zu\n",
          iteration, bytesConsumed());
      return;

    case ERROR:
      JSON_DEBUG_ERROR("JSONParserBase: error at byte %zu\n", bytesConsumed());
      print_state(iteration);
      return;

    case STOPPED:
      JSON_DEBUG_INFO("JSONParserBase: stopped by callback\n");
      return;

    default:
      return;
    }
  }
}

// ── Méthodes d'assignation (logique identique à JSONParser) ───

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_same_type(PV &pv, V &v) {
  if (v != pv) {
    v = pv;
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_convertible(PV &pv, V &v) {
  if (v != pv) {
    v = static_cast<V>(pv);
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_integral_to_integral(PV &pv,
                                                                     V &v) {
  V new_value = clamp_to_max<PV, V>(pv);
  if (v != new_value) {
    v = new_value;
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_infinity_to_integral(PV &,
                                                                     V &v) {
  if constexpr (std::is_integral_v<V>) {
    V nv = std::numeric_limits<V>::max();
    if (v != nv) {
      v = nv;
      return ParseValueResult::VALUE_UPDATED;
    }
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor>::assign_string_view_to_char_array(PV &pv, V &v) {
  if (memcmp(v, pv.data(), pv.length()) == 0) {
    return ParseValueResult::NO_RESULT;
  }

  size_t len = std::min(pv.length(), sizeof(v) - 1);
  std::memcpy(v, pv.data(), len);
  v[len] = '\0';

  return ParseValueResult::VALUE_UPDATED;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_null_ptr_to_pointer(PV &,
                                                                    V &v) {
  if (v != nullptr) {
    v = nullptr;
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::assign_array_to_array(V &pv, V &v) {
  return copy_array(v, pv) ? ParseValueResult::VALUE_UPDATED
                           : ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::assign_string_view_to_unsigned_array(
    std::string_view pv, V &v) {
  return copy_hex_be_to_h(v, pv.data(), pv.length())
             ? ParseValueResult::VALUE_UPDATED
             : ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename PV>
ParseValueResult
JSONParserBase<Cursor>::assign_callback_object(const PV &pv,
                                               JSONCallbackObject &cb) {
  cb.run(pv);
  if (cb.stop) {
    _state = STOPPED;
  }

  return ParseValueResult::VALUE_UPDATED;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_not_handled(PV &pv, V &v) {
  JSON_DEBUG_TYPES("Could not assign value from %s to %s\n", pv, v);
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor>::assign_parsed_value_to_value(PV &pv,
                                                                      V &v) {
  JSON_DEBUG_TYPES("Assign %s to %s\n", pv, v);
  ParseValueResult result = ParseValueResult::NO_RESULT;
  if constexpr (std::is_same_v<PV, V> &&
                is_container_from_list<V, arguments_array_types>::value &&
                container_info<V>::dimensions == 1) {
    result |= ParseValueResult::VALUE_CONVERTED | assign_array_to_array(pv, v);
  } else if constexpr (std::is_same_v<PV, V>) {
    result |= ParseValueResult::VALUE_CONVERTED | assign_same_type(pv, v);
  } else if constexpr (std::is_convertible_v<PV, V> && std::is_integral_v<PV> &&
                       std::is_integral_v<V>) {
    result |=
        ParseValueResult::VALUE_CONVERTED | assign_integral_to_integral(pv, v);
  } else if constexpr (std::is_convertible_v<PV, V> &&
                       std::is_floating_point_v<PV>) {
    result |= ParseValueResult::VALUE_CONVERTED | assign_convertible(pv, v);
  } else if constexpr (std::is_same_v<PV, std::string_view> &&
                       is_char_array_v<V>) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_string_view_to_char_array(pv, v);
  } else if constexpr (std::is_same_v<PV, NullType> && std::is_pointer_v<V>) {
    result |=
        ParseValueResult::VALUE_CONVERTED | assign_null_ptr_to_pointer(pv, v);
  } else if constexpr (std::is_same_v<PV, NaNType>) {
    return result;
  } else if constexpr (std::is_same_v<PV, InfinityType>) {
    result |=
        ParseValueResult::VALUE_CONVERTED | assign_infinity_to_integral(pv, v);
  } else if constexpr (std::is_same_v<PV, std::string_view> &&
                       is_uint_array_v<V>) {
    result |= ParseValueResult::VALUE_CONVERTED |
              assign_string_view_to_unsigned_array(pv, v);
  } else if constexpr (std::is_same_v<V, JSONCallbackObject>) {
    result |= ParseValueResult::VALUE_CONVERTED | assign_callback_object(pv, v);
  } else if constexpr (std::is_same_v<V, UnknownValueType>) {
    return result;
  } else {
    result |= assign_not_handled(pv, v);
  }
  return result;
}

template <typename Cursor>
template <class From, class To>
constexpr To JSONParserBase<Cursor>::clamp_to_max(From v) {
  if constexpr (std::is_signed_v<From> && std::is_unsigned_v<To>) {
    if (v < 0)
      return 0;
  }
  // Upper-bound comparison is only safe when To::max fits in From without
  // overflow.  That holds when sizeof(To) < sizeof(From), or when they are
  // the same size and neither is signed→unsigned (which would make
  // To::max > From::max and overflow the cast).
  constexpr bool upper_safe =
      (sizeof(To) < sizeof(From)) ||
      (sizeof(To) == sizeof(From) &&
       !(std::is_signed_v<From> && std::is_unsigned_v<To>));
  if constexpr (upper_safe) {
    if (static_cast<From>(std::numeric_limits<To>::max()) < v)
      return std::numeric_limits<To>::max();
  }
  // Lower-bound comparison is only meaningful for signed To and only safe
  // when To::min fits in From (i.e. sizeof(To) < sizeof(From) for signed).
  if constexpr (std::is_signed_v<To> && std::is_signed_v<From> &&
                sizeof(To) < sizeof(From)) {
    if (static_cast<From>(std::numeric_limits<To>::min()) > v)
      return std::numeric_limits<To>::min();
  }
  return static_cast<To>(v);
}

// ── parse_array ──────────────────────────────────────────────

// template <typename Cursor>
// template <typename T>
// enable_if_t<is_derived_json_data_container_v<T>, ParseValueResult>
// JSONParserBase<Cursor>::parse_array(T &arg_value) {
//   JSON_DEBUG_INFO("JSONParserBase::parse_array with derived JSONObject
//   objects\n"); return parse_array(arg_value);
// }

template <typename Cursor>
ParseValueResult JSONParserBase<Cursor>::parse_array(UnknownValueType) {
  std::vector<UnknownValueType> tmp;
  return parse_array(tmp);
}

template <typename Cursor>
template <typename V>
enable_if_t<container_info<V>::is_container ||
                std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>,
            ParseValueResult>
JSONParserBase<Cursor>::parse_array(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_array\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (!is_array_start()) {
    return result;
  }

  _cursor.advance();

  uint32_t i = 0;

  while (i < JSON::MAX_ARRAY_LENGTH) {
    skip_spaces();
    result |= parse_into_array_at_index(arg_value, i);

    if (_state == STOPPED) {
      return ParseValueResult::VALUE_PARSED;
    }

    if (!result.parsed()) {
      JSON_DEBUG_WARNING(
          "JSONParserBase::parse_array: cannot parse value at index %zu\n", i);
      return ParseValueResult::NO_RESULT;
    }

    skip_spaces();

    if (!cursor_scan_char(_cursor, JSON_COMMA_CHARACTER, true)) {
      JSON_DEBUG_WARNING("JSONParserBase::parse_array: no comma at index %zu, "
                         "assuming end of array\n",
                         i);
      break;
    }

    i++;
  }

  // if (_current_char() == '\0') {
  //   JSON_DEBUG_WARNING("END OF BUFFER OR STREAM REACHED\n");
  // }

  if (!cursor_scan_char(_cursor, JSON_ARRAY_END_CHARACTER, true)) {
    JSON_DEBUG_WARNING("JSONParserBase::parse_array: no array end\n");
    _state = ERROR;
    return ParseValueResult::NO_RESULT;
  }

  skip_spaces();

  return ParseValueResult::VALUE_PARSED;
}

template <typename Cursor>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(JSONCallbackObject &cb,
                                                  uint32_t index) {
  cb.setArrayIndex(index);

  if (_is_top_level_array) {
    return parse_object(cb);
  }

  return parse_into_value(cb);
}

template <typename Cursor>
template <typename T, size_t N>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(T (&array)[N],
                                                  uint32_t index) {
  if (index >= N) {
    JSON_DEBUG_WARNING(
        "JSONParserBase::parse_into_array_at_index: %zu overflow", index);
    T dummy;
    return parse_into_value(dummy);
  }

  return parse_into_value(array[index]);
}

template <typename Cursor>
template <typename T>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(std::vector<T> &array,
                                                  uint32_t /*index*/) {
  T value{};
  ParseValueResult r = parse_into_value(value);
  if (r & ParseValueResult::VALUE_PARSED)
    array.push_back(value);
  return r;
}

template <typename Cursor>
template <typename T, size_t N>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(std::array<T, N> &array,
                                                  uint32_t index) {
  if (index >= N) {
    T dummy;
    return parse_into_value(dummy);
  }

  return parse_into_value(array[index]);
}

// ── parse_object ──────────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_object(V &arg_value) {
  JSON_DEBUG_TYPES("JSONParser::parse_object into %s\n", arg_value);
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (!is_object_start()) {
    return ParseValueResult::NO_RESULT;
  }

  std::string_view name = (_key_buf[0] == '\0' || _key_length == 0)
                              ? std::string_view("$ROOT", 5)
                              : copy_to_sv(_key_buf, _key_length);
  JSON_DEBUG_INFO("Will parse object '%.*s'\n", (int)name.length(),
                  name.data());
  JSON_DEBUG_INFO("Cursor position is now at %zu\n", bytesConsumed());
  JSON::ParseResult r = arg_value.fromJSON(name, _cursor);

#if JSON_DEBUG_LEVEL > 0
  JSON_DEBUG_INFO("In previous JSONParser %.*s parse_object result: ",
                  (int)_name.length(), _name.data());
  r.print();
  JSON_DEBUG_INFO("Cursor position is now at %zu\n", bytesConsumed());
#endif

  if (r.error != NO_ERROR) {
    JSON_DEBUG_TYPES("In previous JSONParser::parse_object error parsing %s :",
                     arg_value);
    JSON_DEBUG_INFO("%s\n", errorToString((ParserError)r.error));
    _state = END;
    return ParseValueResult::NO_RESULT;
  }

  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    if (r.stopped) {
      JSON_DEBUG_INFO("JSONParser::parse_object parsing stopped\n");
      _state = STOPPED;
    }
  }

  result |= ParseValueResult::VALUE_PARSED | ParseValueResult::VALUE_UPDATED |
            ParseValueResult::VALUE_CONVERTED;

  return result;
}

// ── parse_any ─────────────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_any(V arg_value) {
  ParseValueResult result = ParseValueResult::NO_RESULT;
  result = parse_string(arg_value);
  if (result.parsed())
    return result;
  result = parse_numeric(arg_value);
  if (result.parsed())
    return result;
  result = parse_bool(arg_value);
  if (result.parsed())
    return result;
  result = parse_null(arg_value);
  if (result.parsed())
    return result;
  if (is_array_start())
    return parse_array(arg_value);
  if (is_object_start())
    return parse_object(arg_value);

  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
void JSONParserBase<Cursor>::print_state(size_t iteration) {
  // We cannot print the state of a StreamCursor because it is not seekable.
  if constexpr (std::is_same_v<Cursor, const PointerCursorReader>) {
    size_t length = std::min(_cursor.size(), JSON::DEBUG_COLUMN_WIDTH);

    [[maybe_unused]] size_t col_number = bytesConsumed() / length;
    [[maybe_unused]] size_t col_pos = bytesConsumed() % length;
    [[maybe_unused]] const char *dots = (_cursor.size()) > length ? "..." : "";

    [[maybe_unused]] const char *color =
        (_state == ERROR) ? "\x1b[31m" : "\x1b[32m";
    [[maybe_unused]] const char *error =
        (_state == ERROR) ? errorToString(_lastError) : "";
    [[maybe_unused]] const char *errorValueType =
        (_state == ERROR) ? parsedValueTypeToString(_lastParseValueResult) : "";

    char *output = static_cast<char *>(malloc(length));
    strncpy(output, _cursor.start() + col_number * length, length);

    // REPLACE \n with ' ' in output
    // replace(output, old_chars, new_char);
    replace_endl(output, length);

    DEBUG_PRINTF("Parser '%.*s': %.*s %s pos=%zu it=%zu, p=%p\n%s%*c%s %s %s "
                 "key='%.*s' \x1b[0m\n",
                 (int)_name.length(), _name.data(), (int)length,
                 (const char *)output, dots, bytesConsumed(), iteration, this,
                 color, (int)(11 + _name.length() + col_pos + 1), '^',
                 get_state_name().data(), error, errorValueType,
                 (int)_key_length, _key_buf);

    free(output);
  }
}

template <typename Cursor>
std::string_view JSONParserBase<Cursor>::get_state_name() {
  switch (_state) {
  case IDLE:
    return "IDLE";
    break;
  case KEY:
    return "KEY";
    break;
  case COLON:
    return "COLON";
    break;
  case VALUE:
    return "VALUE";
    break;
  case COMMA:
    return "COMMA";
    break;
  case END:
    return "END";
    break;
  case ERROR:
    return "ERROR";
    break;
  case STOPPED:
    return "STOPPED";
    break;
  default:
    return "UNKNOWN";
  }
}

template <typename Cursor>
const char *JSONParserBase<Cursor>::errorToString(ParserError error) {
  switch (error) {
  case ParserError::NO_ERROR:
    return "NO_ERROR";
  case ParserError::NO_OBJECT_START:
    return "NO_OBJECT_START";
  case ParserError::INVALID_KEY:
    return "INVALID_KEY";
  case ParserError::NO_COLON:
    return "NO_COLON";
  case ParserError::INVALID_VALUE:
    return "INVALID_VALUE";
  case ParserError::NO_COMMA:
    return "NO_COMMA";
  case ParserError::INVALID_OBJECT:
    return "INVALID_OBJECT";
  default:
    return "UNKNOWN";
  }
}

template <typename Cursor>
const char *
JSONParserBase<Cursor>::parsedValueTypeToString(ParseValueResult result) {
  uint16_t type = result.valueType();

  switch (type) {
  case ParseValueResult::UNKNOWN:
    return "UNKNOWN";
  case ParseValueResult::BOOLEAN:
    return "BOOLEAN";
  case ParseValueResult::INTEGER:
    return "INTEGER";
  case ParseValueResult::FLOAT:
    return "FLOAT";
  case ParseValueResult::STRING:
    return "STRING";
  case ParseValueResult::ARRAY:
    return "ARRAY";
  case ParseValueResult::OBJECT:
    return "OBJECT";
  case ParseValueResult::POINTER:
    return "POINTER";
  default:
    return "UNKNOWN";
  }
}
// ============================================================
//  Alias publics
// ============================================================

// JSONParser : version originale basée sur PointerCursor
// (remplace la classe JSONParser existante — même interface)
using JSONParser = JSONParserBase<const JSON::PointerCursorReader>;
using JSONStreamParser = JSONParserBase<JSON::StreamCursor>;
