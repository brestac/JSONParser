#pragma once

// JSONParserBase.h

#include <limits>

#include "ParseDispatchTable.h"
#include "StaticString.h"
#include "types.h"
#include "utils.h"
#ifdef __GXX_RTTI
#include "demangled.h"
#endif
#ifndef ARDUINO
#include "../include/ArduinoCompat.h"
#endif

using namespace std;
using namespace JSON;
struct Parser {};
// ============================================================
//  JSONParserBase<Cursor>
//  Toute la logique du parser, paramétrée uniquement par le
//  type de curseur (PointerCursor ou StreamCursor).
// ============================================================
template <typename Cursor, bool UseMask, typename TargetT = Cursor>
class JSONParserBase {
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

  // ── Constructeur PointerCursor ─
  explicit JSONParserBase(const char *name, Cursor &cursor)
      : _cursor(cursor), _bytesConsumed(cursor.bytesConsumed()), _state(IDLE),
        _automask(false), _keyMask(0), _nParsed(0), _nMatched(0),
        _nConverted(0), _nUpdated(0),
        _is_top_level_array(false) /*, _nArgs(0)*/,
        _lastError(ParserError::NO_ERROR),
        _lastParseError(0), _key_length(0),
        _key_buf(new char[JSON::MAX_KEY_LENGTH + 1]{}),
        _val_buf(new char[JSON::MAX_VALUE_LENGTH + 1]{}) {
    JSON_DEBUG_COLOR(COLOR_BLUE, "JSONParserBase(pointer) '%.*s' created\n",
                     (int)strlen(name), name);
    strncpy(_name, name, sizeof(_name));
    _name[sizeof(_name) - 1] = '\0';
    _cursor.depth++;
  }

  ~JSONParserBase() {
    // Do not destroy the Cursor because it may be used by a parent parser.
    JSON_DEBUG_WARNING("JSONParserBase '%s' ", _name);
    reset();
    _cursor.depth--;
    JSON_DEBUG_WARNING("destroyed\n");
    GLOBAL_PARSER_SIZE -= sizeof(*this);
  }

  // ── API publique (identique à JSONParser) ─────────────────

  template <typename T>
  enable_if_t<is_derived_json_data_container_v<T>, void> parse(T &jsonObjects);

  template <typename... Args> void parse(Args &&...args);

  size_t parsed_length() { return _cursor.bytesConsumed() - _bytesConsumed; }
  ParserState state() { return _state; }
  ParserError error() { return _lastError; }
  ParseValueResult parseError() { return _lastParseError; }

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

  template <typename T, size_t N>
  ParseValueResult parse_into_array_at_index(T (&array)[N], uint32_t index);

  template <typename T>
  ParseValueResult parse_into_array_at_index(std::vector<T> &array,
                                             uint32_t index);

  template <typename T, size_t N>
  ParseValueResult parse_into_array_at_index(std::array<T, N> &array,
                                             uint32_t index);

  template <typename V>
  enable_if_t<container_info<V>::is_container ||
                  std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>,
              ParseValueResult>
  parse_array(V &arg_value);

  // Accessors
  size_t nParsed() { return _nParsed; }
  size_t nMatched() { return _nMatched; }
  size_t nConverted() { return _nConverted; }
  size_t nUpdated() { return _nUpdated; }
  uint32_t keyMask() { return _keyMask; }
  bool automask() { return _automask; }
  void setAutomask(bool automask) { _automask = automask; }
  bool stopped() { return _state == STOPPED; }
  void setName(const char *name) { strncpy(_name, name, sizeof(_name)); }
  std::string_view name() { return std::string_view(_name); }

private:
  Cursor &_cursor; // ← reference: shared across nested parsers
  size_t _bytesConsumed;
  ParserState _state;
  bool _automask;
  uint8_t _keyMask;
  uint8_t _nParsed;
  uint8_t _nMatched;
  uint8_t _nConverted;
  uint8_t _nUpdated;
  bool _is_top_level_array;
  // uint8_t _nArgs;
  ParserError _lastError;
  ParseValueResult _lastParseError;
  char _name[12];
  uint8_t _key_length;
  std::unique_ptr<char[]> _key_buf;
  std::unique_ptr<char[]> _val_buf;

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
  template <typename V> bool scan_escaped_string(std::string_view &sv);
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
  // ParseValueResult skip_to_array_end();
  ParseValueResult skip_to_object_end();

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
};

// ============================================================
//  Implémentation des méthodes
// ============================================================

template <typename Cursor, bool UseMask, typename TargetT>
void JSONParserBase<Cursor, UseMask, TargetT>::reset() {
  // _cursor is passed from parser to parser and should not be reset
  //  _bytesConsumed = 0;
  _automask = false;
  _keyMask = 0;
  _nParsed = 0;
  _nMatched = 0;
  _nConverted = 0;
  _nUpdated = 0;
  _state = IDLE;
  _is_top_level_array = false;
  // _nArgs = 0;
  _lastError = ParserError::NO_ERROR;
  _lastParseError = ParseValueResult();
  _name[0] = '\0';
  _key_length = 0;
  _key_buf.get()[0] = '\0';
  _val_buf[0] = '\0';
}

template <typename Cursor, bool UseMask, typename TargetT>
void JSONParserBase<Cursor, UseMask, TargetT>::set_state(ParserState s) {
  if (_state == END || _state == ERROR || _state == STOPPED)
    return;

  _state = s;

  if (_state == COMMA) {
    _reset_key();
  }
}

template <typename Cursor, bool UseMask, typename TargetT>
void JSONParserBase<Cursor, UseMask, TargetT>::_reset_key() {
  _key_buf.get()[0] = '\0';
  _key_length = 0;
}

// ── parse_key ────────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
bool JSONParserBase<Cursor, UseMask, TargetT>::parse_key() {
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true)) {
    _reset_key();
    return false;
  }

  // Pour StreamCursor on doit copier la clé dans un buffer local.
  // Pour PointerCursor on peut pointer directement (comportement original).
  // On utilise un buffer statique court pour la clé.
  size_t n = 0;

  while (n < JSON::MAX_KEY_LENGTH) {
    CHECK_LOOP(false);
    
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

    _key_buf.get()[n++] = ch;
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

  JSON_DEBUG_INFO("JSONParserBase::parse_key '%.*s'\n", (int)_key_length,
                  _key_buf.get());
  return true;
}

// ── parse_colon ───────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
bool JSONParserBase<Cursor, UseMask, TargetT>::parse_colon() {
  cursor_skip_spaces(_cursor);
  return cursor_scan_char(_cursor, JSON_COLON_CHARACTER, true);
}

// ── parse_comma ───────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
bool JSONParserBase<Cursor, UseMask, TargetT>::parse_comma() {
  return cursor_scan_char(_cursor, JSON_COMMA_CHARACTER, true);
}

// ── scan_digits ───────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
size_t
JSONParserBase<Cursor, UseMask, TargetT>::scan_digits(size_t max_length) {
  size_t n = 0;
  while (max_length == 0 || n < max_length) {
    CHECK_LOOP(0);
    
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

template <typename Cursor, typename TargetT, typename TargetV>
static bool needs_pool(bool unescaped) {
  // JSON_DEBUG_COLOR(COLOR_MAGENTA, "needs_pool<%s, %s>(%d)\n",
  // typeid(Cursor).name(), typeid(TargetT).name(), unescaped);

  // Quand avons-nous besoin d'un pool ?
  // 1. Quand le string est échappé (unescaped == true) dans tous les cas
  // 2. Quand le curseur est un StreamCursor et que la cible est une string_view

  if constexpr (std::is_same_v<remove_cvref_t<Cursor>, StreamCursor> &&
                std::is_same_v<remove_cvref_t<TargetV>, std::string_view>) {
    return true;
  } else {
    return unescaped;
  }
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
bool JSONParserBase<Cursor, UseMask, TargetT>::scan_escaped_string(
    std::string_view &sv) {
  using Pool = StaticString<TargetT>;

  bool inEscape = false;
  bool unescaped = false;
  size_t n = 0;
  Pool::ensure_pool_size(1);
  char *pool_start_ptr = Pool::current_pos();
  
  while (true) {
    CHECK_LOOP(false);
    
    unsigned char c = _cursor.peek();

    if (c < 0) {
      break;
    }

    char ch = static_cast<char>(c);

    if (inEscape) {
      inEscape = false;
      if (ch != JSON_QUOTE_CHARACTER) {
        Pool::write_at(ch, n);
      } else {
        unescaped = true;
      }
    } else {
      if (ch == JSON_ESCAPE_CHARACTER) {
        inEscape = true;
        _cursor.advance();
        continue;
      } else if (ch == JSON_QUOTE_CHARACTER) {
        break;
      }
    }

    Pool::write_at(ch, n);

    _cursor.advance();
    n++;
  }

  if (n >= JSON::MAX_VALUE_LENGTH) {
    return false;
  }

  // Le pool est alloué à l'avance uniquement pour les StreamCursor avec des
  // valeurs std::string_view dans JSONParser _parse_impl<true, StreamCursor,
  // UserStruct>
  if (needs_pool<Cursor, TargetT, V>(unescaped)) {
    Pool::increment_values_counter();
    Pool::move_offset(n);
    sv = std::string_view(pool_start_ptr, n);
  } else {
    sv = std::string_view(pool_start_ptr, n);
  }

  return true;
}

// For the PointerCursorReader, we scan the string as we would normally do for
// non escaped strings. Then we peek back by one and check if the previous
// character was an escape character. If it was, we go back to the start of the
// string and scan it again, this time handling escape sequences. Then we
// advance the cursor by the length of the string.
// Au lieu de deux spécialisations totales de parse_string,
// une seule implémentation dans le template primaire :

template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_string(V &arg_value) {
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  std::string_view parsed_value;
  // Plusieurs cas: Cursor = PointerCursorReader ou StreamCursor
  // TargetT = Cursor si appel user depuis JSON::parse, UserStruct si appel user
  // depuis fromJSON, JSONCallbackObject si appel depuis parse avec callback
  // UnknownValueType si appel depuis parse avec UnknownValueType
  if constexpr (std::is_same_v<Cursor, const PointerCursorReader>) {
    // Chemin rapide : pointer direct dans le buffer
    const char *str_start = _cursor.ptr();
    if (!cursor_scan_until(_cursor, JSON_QUOTE_CHARACTER, MAX_VALUE_LENGTH,
                           true, false))
      return ParseValueResult::NO_RESULT;

    parsed_value = std::string_view(str_start, _cursor.ptr() - str_start);

    if (_cursor.peek(-1) == JSON_ESCAPE_CHARACTER) {
      _cursor.go_to(str_start);
      if (!scan_escaped_string<V>(parsed_value))
        return ParseValueResult::NO_RESULT;
    }
  } else if constexpr (std::is_same_v<Cursor, StreamCursor>) {
    // StreamCursor : pool obligatoire
    if (!scan_escaped_string<V>(parsed_value))
      return ParseValueResult::NO_RESULT;
  } else {
    JSON_DEBUG_ERROR("JSONParserBase::parse_string: unsupported cursor type\n");
    return ParseValueResult::NO_RESULT;
  }

  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  return assign_parsed_value_to_value(parsed_value, arg_value) | ParseValueResult::STRING_PARSED;
}
// ── parse_integer ────────────────────────────────────────────
// Extrait les digits dans un buffer local, puis appelle strtol.
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_integer(V &arg_value) {
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
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_floating_point(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_floating_point\n");
  return parse_numeric<V, double>(arg_value);
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename V, typename Type>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_numeric(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_numeric\n");

  char *start;

  if constexpr (std::is_same_v<Cursor, const PointerCursorReader>) {
    start = const_cast<char *>(_cursor.ptr());
  } else {
    static char tmp[64];
    size_t len = _cursor.peekToken(tmp, sizeof(tmp) - 1);
    if (len == 0)
      return ParseValueResult::PARSE_ERROR_NUMERIC;

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
    return ParseValueResult::PARSE_ERROR_NUMERIC;

  _cursor.advance(consumed);

  if constexpr (std::is_integral_v<Type>) {
    if (cursor_scan_char(_cursor, '.', true)) {
      JSON_DEBUG_WARNING("JSONParserBase::parse_numeric integer: found extra "
                         "digits after '.'\n");
      scan_digits(JSON::MAX_VALUE_LENGTH);
    }
  }

  return assign_parsed_value_to_value(parsed_value, arg_value) | ParseValueResult::NUMERIC_PARSED;
}

// ── parse_bool ───────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_bool(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_bool\n");
  if (cursor_scan_keyword(_cursor, JSON_FALSE, true)) {
    bool pv = false;
    return assign_parsed_value_to_value(pv, arg_value) | ParseValueResult::BOOLEAN_PARSED;
  }
  
  if (cursor_scan_keyword(_cursor, JSON_TRUE, true)) {
    bool pv = true;
    return assign_parsed_value_to_value(pv, arg_value) | ParseValueResult::BOOLEAN_PARSED;
  }
  
  return ParseValueResult::PARSE_ERROR_BOOLEAN;
}

// ── parse_null ───────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_null(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_null\n");
  if (!cursor_scan_keyword(_cursor, JSON_NULL, true)) {
    return ParseValueResult::PARSE_ERROR_NULL;
  }
  
  NullType pv;
  return assign_parsed_value_to_value(pv, arg_value) | ParseValueResult::NULL_VALUE_PARSED;
}

// ── parse_nan ────────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_nan(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_nan\n");
  if (!cursor_scan_keyword(_cursor, JSON_NAN, true)) {
    return ParseValueResult::PARSE_ERROR_NUMERIC;
  }
    
  NullType pv;
  return assign_parsed_value_to_value(pv, arg_value) | ParseValueResult::NUMERIC_PARSED;
}

// ── parse_infinity ───────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_infinity(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_infinity\n");
  if (!cursor_scan_keyword(_cursor, JSON_INFINITY, true)) {
    return ParseValueResult::PARSE_ERROR_NUMERIC;
  }

  InfinityType pv;
  return assign_parsed_value_to_value(pv, arg_value) | ParseValueResult::NUMERIC_PARSED;
}

// ── parse_numeric ────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_numeric(V &arg_value) {
  if constexpr (std::is_same_v<V, JSONCallbackObject> ||
                std::is_same_v<V, UnknownValueType>) {
    bool ok = parse_floating_point(arg_value).parsed() ||
              parse_integer(arg_value).parsed() ||
              parse_nan(arg_value).parsed() ||
              parse_infinity(arg_value).parsed();
    return ok ? ParseValueResult::NUMERIC_PARSED : ParseValueResult::PARSE_ERROR_NUMERIC;
  } else if constexpr (std::is_floating_point_v<remove_cvref_t<V>>) {
    return parse_floating_point(arg_value);
  } else if constexpr (std::is_integral_v<remove_cvref_t<V>> &&
                       !std::is_same_v<remove_cvref_t<V>, bool>) {
    return parse_integer(arg_value);
  }

  return ParseValueResult::PARSE_ERROR_NUMERIC;
}

// ── parse_unknown_value ───────────────────────────────────────
// Saute une valeur JSON sans la parser (objet, tableau, littéral...)
template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_unknown_value() {
  JSON_DEBUG_INFO("JSONParserBase::parse_unknown_value\n");
  [[maybe_unused]] size_t iteration = 0;
  int depth = 0;
  bool inString = false;
  bool escape = false;

  while (true) {
    CHECK_LOOP(ParseValueResult::PARSE_ERROR_UNKNOWN);
    
    int c = _cursor.peek();

    if (c < 0)
      return ParseValueResult::PARSE_ERROR_UNKNOWN;

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
                  iteration);

  return ParseValueResult::OBJECT_PARSED;
}

template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::skip_to_object_end() {
  JSON_DEBUG_INFO("JSONParserBase::skip_to_object_end\n");
  // We are in the middle of an object before the comma, we need to skip to the
  // end of the object We use parse_unknown_value to skip the each value until
  // we find the end of the object

  while (true) {
    CHECK_LOOP(ParseValueResult::PARSE_ERROR_OBJECT);

    if (is_object_end()) {
      //_cursor.advance();
      break;
    }

    ParseValueResult r = parse_unknown_value();

    if (r.parsed()) {
      if (is_object_end()) {
        //_cursor.advance();
        break;
      } else {
        skip_spaces();

        if (!cursor_scan_char(_cursor, JSON_COMMA_CHARACTER, true)) {
          JSON_DEBUG_ERROR("JSONParserBase::skip_to_object_end: no comma\n");
          return ParseValueResult::PARSE_ERROR_OBJECT;
        }
      }

      continue;
    } else {
      JSON_DEBUG_ERROR("JSONParserBase::skip_to_object_end: cannot parse value "
                       "%s for key #%zu\n",
                       errorToString(r), iteration);
      return ParseValueResult::PARSE_ERROR_OBJECT;
    }
  }

  return ParseValueResult::OBJECT_PARSED;
}
/*
template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::skip_to_array_end() {
  JSON_DEBUG_INFO("JSONParserBase::skip_to_array_end\n");
  // We are in the middle of an array before the comma, we need to skip to the
end of the array
  // We use parse_unknown_value to skip the each value until we find the end of
the array size_t iterations = 0;

  while (true) {
    if (iterations > MAX_ITERATIONS) {
      JSON_DEBUG_ERROR(
          "JSONParserBase::skip_to_array_end: too many iterations\n");
      return ParseValueResult::NO_RESULT;
    }

    iterations++;

    ParseValueResult r = parse_unknown_value();
    if (r.parsed()) {
      if (is_array_end()) {
        _cursor.advance();
        break;
      }
      else {
        // skip the comma
        skip_spaces();

        if (!cursor_scan_char(_cursor, JSON_COMMA_CHARACTER, true)) {
          JSON_DEBUG_ERROR("JSONParserBase::skip_to_array_end: no comma\n");
          return ParseValueResult::NO_RESULT;
        }
        continue;
      }

    }
    else {
      return ParseValueResult::NO_RESULT;
    }

  }

  return ParseValueResult::VALUE_PARSED;
}
*/
// ── parse_value (callback) ────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_value(JSONCallbackObject &cb) {
  JSON_DEBUG_WARNING(
      "JSONParserBase<Cursor, UseMask, TargetT>::parse_value with callback\n");

  cb.setKey(_key_buf.get(), _key_length);

  if (_is_top_level_array) {
    JSON_DEBUG_INFO("JSONParserBase<Cursor, UseMask, TargetT>::parse_value top "
                    "level array\n");
    return parse_array(cb) | ParseValueResult::KEY_FOUND;
  }

  return parse_into_value(cb) | ParseValueResult::KEY_FOUND;
}

template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult JSONParserBase<Cursor, UseMask, TargetT>::parse_value(
    UnknownValueType &unknown) {
  JSON_DEBUG_WARNING("JSONParserBase<Cursor, UseMask, TargetT>::parse_value "
                     "UnknownValueType\n");

  return parse_into_value(unknown) | ParseValueResult::KEY_FOUND;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename TupleT, typename TableT>
std::enable_if_t<(std::tuple_size<TupleT>::value == 1), ParseValueResult>
JSONParserBase<Cursor, UseMask, TargetT>::parse_value(TableT & /*table*/,
                                                      TupleT &args) {
  return parse_value(std::get<0>(args));
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename TupleT, typename TableT>
std::enable_if_t<(std::tuple_size<TupleT>::value > 1), ParseValueResult>
JSONParserBase<Cursor, UseMask, TargetT>::parse_value(TableT &table,
                                                      TupleT &args) {
  constexpr size_t NPairs = std::tuple_size<TupleT>::value / 2;
  const std::string_view parsed_key(_key_buf.get(), _key_length);
  const StaticEntry *entry = table.find(hash32(parsed_key));

  if (!entry) {
    JSON_DEBUG_WARNING("JSONParserBase<Cursor, UseMask, TargetT>::parse_value "
                       "key '%.*s' not found\n",
                       (int)parsed_key.length(), parsed_key.data());
    return ParseValueResult::NO_RESULT;
  }

  ParseValueResult result;
  result |= ParseValueResult::KEY_FOUND;
  
  result |= dispatch_by_index(entry->arg_index, *this, args,
                              std::make_index_sequence<NPairs>{});

  if (result.updated()) {
    if constexpr (UseMask) {
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
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_into_value(V &arg_value) {
  JSON_DEBUG_TYPES(
      "JSONParserBase<Cursor, UseMask, TargetT>::parse_into_value %s\n",
      arg_value);
  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_same_v<V, bool>) {
    return parse_bool(arg_value) | ParseValueResult::BOOLEAN_PARSED;
  } else if constexpr (std::is_floating_point_v<V>) {
    return parse_floating_point(arg_value) | ParseValueResult::FLOAT_PARSED;
  } else if constexpr (std::is_integral_v<V>) {
    return parse_integer(arg_value) | ParseValueResult::NUMERIC_PARSED;
  } else if constexpr (std::is_same_v<V, std::string_view> ||
                       is_char_array_v<V>) {
    return parse_string(arg_value) | ParseValueResult::STRING_PARSED;
  } else if constexpr (is_uint_array_v<V>) {
    ParseValueResult result = parse_string(arg_value);

    if (result.parsed()) {
      return result | ParseValueResult::STRING_PARSED;
    }

    result = parse_array(arg_value);
    return result.parsed() ? result | ParseValueResult::ARRAY_PARSED
                           : ParseValueResult::PARSE_ERROR_ARRAY;
  } else if constexpr (is_container_v<V>) {
    return parse_array(arg_value) | ParseValueResult::ARRAY_PARSED;
  } else if constexpr (std::is_same_v<remove_cvref_t<V>, UnknownValueType>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_base_of_v<JSONObject, remove_cvref_t<V>>) {
    return parse_object(arg_value) | ParseValueResult::OBJECT_PARSED;
  } else if constexpr (std::is_pointer_v<V>) {
    ParseValueResult result = parse_null(arg_value);
    if constexpr (!std::is_const_v<std::remove_pointer_t<V>> &&
                  !std::is_same_v<V, UnknownValueType>) {
      if (!result.parsed() && arg_value != nullptr)
        result |= parse_into_value(*arg_value);
    }
    return result;
  } else {
    return ParseValueResult::PARSE_ERROR_UNKNOWN;
  }
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename T>
enable_if_t<is_derived_json_data_container_v<T>, void>
JSONParserBase<Cursor, UseMask, TargetT>::parse(T &jsonObjects) {
  JSON_DEBUG_INFO("JSONParserBase::parse with derived JSONObject objects\n");
  _is_top_level_array = true;
  parse_array(jsonObjects);
  _state = END;
}

// ── parse (boucle principale) ─────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename... Args>
void JSONParserBase<Cursor, UseMask, TargetT>::parse(Args &&...args) {
  static_assert(sizeof...(Args) > 0, "::parse No arguments provided");
  using TupleT = std::tuple<Args &&...>;
  constexpr size_t NPairs = sizeof...(Args) / 2;

  // Références runtime — reconstruites à chaque appel
  TupleT refs(std::forward<Args>(args)...);

  // Table statique — uniquement hash + index, pas de références
  // static constexpr possible car ne dépend QUE des const char[N]
  // qui sont des littéraux, stables pour toute la spécialisation
  static const StaticDispatchTable<NPairs> table(refs);

  // _nArgs = sizeof...(Args);
  size_t iteration = 0;

  while (!_cursor.eof()) {
    iteration++;

    if (iteration % 64 == 0)
      yield();

    if (iteration >= MAX_ITERATIONS) {
      JSON_DEBUG_ERROR("JSONParserBase::parse: too many iterations\n");
      _state = ERROR;
      _lastError = ParserError::TOO_MANY_ITERATIONS;
      break;
    }

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
        _nParsed++;
        JSON_DEBUG_INFO("nParsed=%zu\n", _nParsed);
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

      _nMatched += r.keyFound() ? 1 : 0;

      if (!r.keyFound()) { // The key was not found in the arguments. This is
                           // not an error. We skip the value.
        r = parse_unknown_value();
      }

      if (r.parsed()) {
        set_state(COMMA);
      } else { // The key was found but the value was not parsed. This is an
               // error.
        _state = ERROR;
        _lastError = ParserError::INVALID_VALUE;
        _lastParseError = r;
      }

      if constexpr (!std::is_same_v<remove_cvref_t<TargetT>,
                                    JSONCallbackObject>) {
        if (_nMatched >= NPairs) {
          JSON_DEBUG_WARNING("JSONParserBase::parse: Parser '%s' depth %zu: "
                             "all keys found,(%zu/%zu) skiping to object end\n",
                             _name, _cursor.depth, _nMatched, NPairs);
          _state = STOPPED;
        }
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
      if constexpr (std::is_same_v<remove_cvref_t<TargetT>,
                                   JSONCallbackObject>) {
        JSON_DEBUG_INFO("JSONParserBase: stopped by callback\n");
        return;
      } else {
        ParseValueResult r = skip_to_object_end();

        if (r.parsed()) {
          _state = END;
        } else {
          _state = ERROR;
          _lastError = ParserError::INVALID_VALUE;
          _lastParseError = r;
        }
        break;
      }

    default:
      return;
    }
  }
}

// ── Méthodes d'assignation (logique identique à JSONParser) ───

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_same_type(PV &pv, V &v) {
  if (v != pv) {
    v = pv;
    return ParseValueResult::VALUE_UPDATED;
  }
  
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_convertible(PV &pv, V &v) {
  if (v != pv) {
    v = static_cast<V>(pv);
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_integral_to_integral(PV &pv,
                                                                      V &v) {
  V new_value = clamp_to_max<PV, V>(pv);
  if (v != new_value) {
    v = new_value;
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_infinity_to_integral(PV &,
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

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_string_view_to_char_array(
    PV &pv, V &v) {
  if (memcmp(v, pv.data(), pv.length()) == 0) {
    return ParseValueResult::NO_RESULT;
  }

  size_t len = std::min(pv.length(), sizeof(v) - 1);
  std::memcpy(v, pv.data(), len);
  v[len] = '\0';

  return ParseValueResult::VALUE_UPDATED;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_null_ptr_to_pointer(PV &,
                                                                     V &v) {
  if (v != nullptr) {
    v = nullptr;
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_array_to_array(V &pv, V &v) {
  return copy_array(v, pv) ? ParseValueResult::VALUE_UPDATED
                           : ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_string_view_to_unsigned_array(
    std::string_view pv, V &v) {
  return copy_hex_be_to_h(v, pv.data(), pv.length())
             ? ParseValueResult::VALUE_UPDATED
             : ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_callback_object(
    const PV &pv, JSONCallbackObject &cb) {
  cb.run(pv);
  if (cb.stop) {
    _state = STOPPED;
  }

  return ParseValueResult::VALUE_UPDATED;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_not_handled(PV & /*pv*/,
                                                             V & /*v*/) {
  JSON_DEBUG_TYPES("Could not assign value from %s to %s\n", "?", "?");
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::assign_parsed_value_to_value(PV &pv,
                                                                       V &v) {
  JSON_DEBUG_TYPES("Assign %s to %s\n", pv, v);
  ParseValueResult result = ParseValueResult::NO_RESULT;
  if constexpr (std::is_same_v<PV, V> &&
                is_container_from_list<V, arguments_array_types>::value &&
                container_info<V>::dimensions == 1) {
    result |= assign_array_to_array(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<PV, V>) {
    result |= assign_same_type(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_convertible_v<PV, V> && std::is_integral_v<PV> &&
                       std::is_integral_v<V>) {
    result |=
        assign_integral_to_integral(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_convertible_v<PV, V> &&
                       std::is_floating_point_v<PV>) {
    result |= assign_convertible(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<PV, std::string_view> &&
                       is_char_array_v<V>) {
    result |= assign_string_view_to_char_array(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<PV, NullType> && std::is_pointer_v<V>) {
    result |=
        assign_null_ptr_to_pointer(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<PV, NaNType>) {
    return result;
  } else if constexpr (std::is_same_v<PV, InfinityType>) {
    result |=
        assign_infinity_to_integral(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<PV, std::string_view> &&
                       is_uint_array_v<V>) {
    result |= assign_string_view_to_unsigned_array(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<V, JSONCallbackObject>) {
    result |= assign_callback_object(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr (std::is_same_v<V, UnknownValueType>) {
    return result;
  } else {
    result |= assign_not_handled(pv, v);
  }
  return result;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <class From, class To>
constexpr To JSONParserBase<Cursor, UseMask, TargetT>::clamp_to_max(From v) {
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
// JSONParserBase<Cursor, UseMask, TargetT>::parse_array(T &arg_value) {
//   JSON_DEBUG_INFO("JSONParserBase::parse_array with derived JSONObject
//   objects\n"); return parse_array(arg_value);
// }

template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_array(UnknownValueType) {
  static std::vector<UnknownValueType> tmp;
  return parse_array(tmp);
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
enable_if_t<container_info<V>::is_container ||
                std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>,
            ParseValueResult>
JSONParserBase<Cursor, UseMask, TargetT>::parse_array(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_array\n");
  ParseValueResult result;

  if (!is_array_start()) {
    return ParseValueResult::PARSE_ERROR_ARRAY;
  }

  _cursor.advance();

  size_t i = 0;

  while (i < MAX_ITERATIONS) {
    skip_spaces();
    result |= parse_into_array_at_index(arg_value, i);

    if (_state == STOPPED) {
      return ParseValueResult::ARRAY_PARSED;
    }

    if (!result.parsed()) {
      JSON_DEBUG_WARNING(
          "JSONParserBase::parse_array: cannot parse value at index %zu\n", i);
      return ParseValueResult::PARSE_ERROR_ARRAY;
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

  if (!cursor_scan_char(_cursor, JSON_ARRAY_END_CHARACTER, true)) {
    JSON_DEBUG_WARNING("JSONParserBase::parse_array: no array end\n");
    _state = ERROR;
    return ParseValueResult::PARSE_ERROR_ARRAY;
  }

  skip_spaces();

  return ParseValueResult::ARRAY_PARSED;
}

template <typename Cursor, bool UseMask, typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_into_array_at_index(
    JSONCallbackObject &cb, uint32_t index) {
  cb.setArrayIndex(index);

  if (_is_top_level_array) {
    return parse_object(cb);
  }

  return parse_into_value(cb);
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename T, size_t N>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_into_array_at_index(
    T (&array)[N], uint32_t index) {
  static UnknownValueType unknown;

  if (index >= N) {
    JSON_DEBUG_WARNING(
        "JSONParserBase::parse_into_array_at_index: %zu overflow", index);
    return parse_into_value(unknown);
  }

  return parse_into_value(array[index]);
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename T>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_into_array_at_index(
    std::vector<T> &array, uint32_t /*index*/) {
  T value{};
  ParseValueResult r = parse_into_value(value);
  if (r.parsed()) {
    array.push_back(value);
  }

  return r;
}

template <typename Cursor, bool UseMask, typename TargetT>
template <typename T, size_t N>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_into_array_at_index(
    std::array<T, N> &array, uint32_t index) {
  static UnknownValueType unknown;
  if (index >= N) {
    JSON_DEBUG_COLOR(COLOR_RED, "Array overflow at index %zu\n", index);
    return parse_into_value(unknown);
  }

  JSON_DEBUG_COLOR(COLOR_GREEN, "Parsing array at index %zu into %s\n", index,
                   typeid(T).name());

  return parse_into_value(array[index]);
}

// ── parse_object ──────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_object(V &arg_value) {
  JSON_DEBUG_TYPES("JSONParser::parse_object into %s\n", arg_value);
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (!is_object_start()) {
    return ParseValueResult::PARSE_ERROR_OBJECT;
  }
  bool key_not_set = _key_buf.get()[0] == '\0' || _key_length == 0;
  const char *name = key_not_set ? "$UNAMED" : _key_buf.get();
  JSON_DEBUG_INFO("Will parse object '%s'\n", name);
  JSON_DEBUG_INFO("Cursor position is now at %zu\n", bytesConsumed());
  JSON::ParseResult r = arg_value.fromJSON(name, _cursor);

#if JSON_DEBUG_LEVEL > 0
  JSON_DEBUG_INFO("In previous JSONParser '%s', parse_object result: ", name);
  r.print();
  JSON_DEBUG_INFO("Cursor position is now at %zu\n", bytesConsumed());
#endif

  if (r.error != NO_ERROR) {
    JSON_DEBUG_TYPES("In previous JSONParser::parse_object error parsing %s :",
                     arg_value);
    JSON_DEBUG_INFO("%s\n", errorToString(r.error));
    _state = END;
    _lastError = r.error;
    _lastParseError = r.parseError;
    return ParseValueResult::PARSE_ERROR_OBJECT;
  }

  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    if (r.stopped) {
      JSON_DEBUG_INFO("JSONParser::parse_object parsing stopped\n");
      _state = STOPPED;
    }
  }

  result |= ParseValueResult::VALUE_CONVERTED;

  return result;
}

// ── parse_any ─────────────────────────────────────────────────
template <typename Cursor, bool UseMask, typename TargetT>
template <typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask, TargetT>::parse_any(V arg_value) {
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

template <typename Cursor, bool UseMask, typename TargetT>
void JSONParserBase<Cursor, UseMask, TargetT>::print_state(size_t iteration) {
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
        (_state == ERROR) ? errorToString(_lastParseError) : "";

    char *output = static_cast<char *>(malloc(length));
    strncpy(output, _cursor.start() + col_number * length, length);

    // REPLACE \n with ' ' in output
    // replace(output, old_chars, new_char);
    replace_endl(output, length);

    DEBUG_PRINTF("Parser '%s': %.*s %s pos=%zu it=%zu, p=%p\n%s%*c%s %s %s "
                 "key='%.*s' \x1b[0m\n",
                 _name, (int)length, (const char *)output, dots,
                 bytesConsumed(), iteration, this, color,
                 (int)(11 + strlen(_name) + col_pos + 1), '^',
                 get_state_name().data(), error, errorValueType,
                 (int)_key_length, _key_buf.get());

    free(output);
  }
}

template <typename Cursor, bool UseMask, typename TargetT>
std::string_view JSONParserBase<Cursor, UseMask, TargetT>::get_state_name() {
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
