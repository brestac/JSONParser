#pragma once

// JSONStreamParser.h
//
// Fournit JSONStreamParser<N> : version de JSONParser qui lit depuis
// un Stream Arduino via un ring buffer de taille N (puissance de 2).
//
// L'implémentation est partagée via JSONParserBase<Cursor>, templatisée
// uniquement sur le type de curseur. L'API publique est identique à
// JSONParser.
//
// Utilisation :
//
//   #include "JSONStreamParser.h"
//
//   // Avec un ring buffer de 256 octets (défaut) :
//   JSONStreamParser<> parser(wifiClient);
//
//   // Avec un ring buffer de 512 octets :
//   JSONStreamParser<512> parser(serial);
//
//   uint32_t mask = 0;
//   float temp = 0;
//   char  name[32] = {};
//   parser.parse("temperature", temp, "name", name);

#include <functional>
#include <string_view>

#include "JSONData.h"
#include "StreamScanner.h"
#include "constants.h"
#include "macros.h"
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

  // ── État public (identique à JSONParser) ─────────────────
  ParserState _state;
  bool _automask;
  uint32_t keyMask;
  size_t nKeys;
  size_t nParsed;
  size_t nConverted;
  size_t nUpdated;

  // ── Constructeur PointerCursor (compatibilité JSONParser) ─
  JSONParserBase(const PointerCursorReader cursor)
      : keyMask(0), nKeys(0), nParsed(0), nConverted(0), nUpdated(0),
        _cursor(cursor), _key_start(nullptr), _key_length(0), _is_top_level_array(false),
        _array_index(0), _nArgs(0) {
    _state = IDLE;
    _automask = false;
    JSON_DEBUG_INFO("JSONParserBase(pointer) created\n");
  }

#ifdef ARDUINO
  // ── Constructeur StreamCursor ─────────────────────────────
  // Used when Cursor = StreamCursor; never called for other cursor types.
  explicit JSONParserBase(StreamCursor &cursor)
      : keyMask(0), nKeys(0), nParsed(0), nConverted(0), nUpdated(0),
        _cursor(cursor), _key_start(nullptr), _key_length(0), _is_top_level_array(false),
        _array_index(0), _nArgs(0) {
    _state = IDLE;
    _automask = false;
    JSON_DEBUG_INFO("JSONParserBase(stream) created\n");
  }
#endif

  ~JSONParserBase() = default;

  // ── API publique (identique à JSONParser) ─────────────────

  //void parse(const JSONCallbackObject& cb);
  
  template <typename T>
  enable_if_t<is_derived_json_data_container_v<T>, void>
  parse(T& jsonObjects);
  
  template <typename... Args>
  void parse(Args &&...args);

  size_t parsed_length() { return _cursor.bytesConsumed(); }
  ParserState get_state() { return _state; }
  bool error() { return _state == ERROR; }
  void setArrayIndex(int i) { _array_index = i; }

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
  ParseValueResult assign_callback_object(PV pv, JSONCallbackObject &cb);

  template <typename PV, typename V>
  ParseValueResult assign_infinity_to_integral(PV &pv, V &v);

  template <class From, class To> constexpr To clamp_to_max(From v);

  // ── Recherche de valeur par clé ────────────────────────────

  inline ParseValueResult searchValueArgumentForKey(size_t idx,
                                                    JSONKey parsed_key);

  template <typename V, typename... Args>
  ParseValueResult searchValueArgumentForKey(size_t idx, JSONKey parsed_key,
                                             JSONKey arg_key, V &arg_value,
                                             Args &&...args);

  template <typename V> ParseValueResult parse_into_value(V &arg_value);

  ParseValueResult parse_into_array_at_index(JSONCallbackObject cb,
                                             size_t index);

  template <typename T, size_t N2>
  ParseValueResult parse_into_array_at_index(T (&array)[N2], size_t index);

  template <typename T>
  ParseValueResult parse_into_array_at_index(std::vector<T> &array,
                                             size_t index);

  template <typename T, size_t N2>
  ParseValueResult parse_into_array_at_index(std::array<T, N2> &array,
                                             size_t index);

  template <typename V>
  enable_if_t<container_info<V>::is_container ||
                  std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>,
              ParseValueResult>
  parse_array(V &arg_value);

private:
  Cursor _cursor; // ← seul membre qui change selon le type
  char *_key_start;
  size_t _key_length;
  bool _is_top_level_array;
  size_t _array_index;
  uint8_t _nArgs;

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

  ParseValueResult parse_value(JSONCallbackObject& cb);

  template <class... Args>
  enable_if_t<args_are_pairs<Args...>, ParseValueResult>
  parse_value(Args &&...args);

  template <typename V> ParseValueResult parse_string(V &v);
  template <typename V, typename Type> ParseValueResult parse_numeric(V &v);
  template <typename V> ParseValueResult parse_floating_point(V &v);
  template <typename V> ParseValueResult parse_integer(V &v);
  template <typename V> ParseValueResult parse_numeric(V &v);
  template <typename V> ParseValueResult parse_bool(V &v);
  template <typename V> ParseValueResult parse_null(V &v);
  template <typename V> ParseValueResult parse_nan(V &v);
  template <typename V> ParseValueResult parse_infinity(V &v);
  //ParseValueResult parse_array(JSONCallbackObject &cb);
  ParseValueResult parse_array(UnknownValueType);

  template <typename V> ParseValueResult parse_object(V &v);
  template <typename V> ParseValueResult parse_any(V v);

  ParseValueResult parse_unknown_value();

  size_t get_position() { return _cursor.bytesConsumed(); }
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

template <typename Cursor>
void JSONParserBase<Cursor>::set_state(ParserState s) {
  if (_state == END || _state == ERROR || _state == STOPPED)
    return;
  _state = s;
  if (_state == COMMA) {
    _key_start = nullptr;
    _key_length = 0;
  }
}

// ── parse_key ────────────────────────────────────────────────
template <typename Cursor> bool JSONParserBase<Cursor>::parse_key() {
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true)) {
    _key_start = nullptr;
    _key_length = 0;
    return false;
  }

  // Pour StreamCursor on doit copier la clé dans un buffer local.
  // Pour PointerCursor on peut pointer directement (comportement original).
  // On utilise un buffer statique court pour la clé.
  static char key_buf[JSON::MAX_KEY_LENGTH + 1];
  size_t n = 0;

  while (n < JSON::MAX_KEY_LENGTH) {
    int c = _cursor.peek(n);
    if (c < 0) {
      _key_start = nullptr;
      _key_length = 0;
      return false;
    }
    char ch = static_cast<char>(c);
    // Valide si dans JSON_KEY_CHARACTERS (ranges a–z A–Z 0–9 _ $)
    bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                 (ch >= '0' && ch <= '9') || ch == '_' || ch == '$';
    if (!valid)
      break;
    key_buf[n++] = ch;
  }
  key_buf[n] = '\0';

  if (n == 0) {
    _key_start = nullptr;
    _key_length = 0;
    return false;
  }

  _cursor.advance(n); // consomme les caractères de la clé

  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true)) {
    _key_start = nullptr;
    _key_length = 0;
    return false;
  }

  _key_start = key_buf;
  _key_length = n;

  JSON_DEBUG_INFO("JSONParserBase::parse_key '%.*s'\n", (int)n, key_buf);
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

// ── parse_string ─────────────────────────────────────────────
// For const PointerCursorReader without escape sequences: constructs a string_view
// pointing directly into the input buffer (zero-copy, avoids static buffer
// aliasing when the same field is parsed into multiple struct instances).
// For StreamCursor or strings containing escape sequences: copies into a
// static buffer (single-parser use only; char[] targets always copy safely).
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_string(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_string\n");
  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  if constexpr (std::is_same_v<remove_cvref_t<Cursor>, PointerCursorReader>) {
    // Peek ahead to find closing quote without advancing, detecting escapes.
    const char *str_start = _cursor.ptr();
    size_t n = 0;
    bool has_escape = false;
    while (n < JSON::MAX_VALUE_LENGTH) {
      int c = _cursor.peek(n);
      if (c < 0)
        return ParseValueResult::NO_RESULT;
      char ch = static_cast<char>(c);
      if (ch == JSON_QUOTE_CHARACTER)
        break;
      if (ch == JSON_ESCAPE_CHARACTER) {
        has_escape = true;
        break;
      }
      n++;
    }
    if (!has_escape) {
      // Zero-copy path: string_view points directly into the JSON input buffer.
      _cursor.advance(n);
      if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
        return ParseValueResult::NO_RESULT;
      std::string_view parsed_value(str_start, n);
      return ParseValueResult::VALUE_PARSED |
             assign_parsed_value_to_value(parsed_value, arg_value);
    }
    // Escape present: fall through to copy path below.
  }

  // Copy path: used for StreamCursor or when escape sequences are present.
  static char val_buf[JSON::MAX_VALUE_LENGTH + 1];
  size_t n = 0;

  while (n < JSON::MAX_VALUE_LENGTH) {
    int c = _cursor.peek();
    if (c < 0)
      return ParseValueResult::NO_RESULT;
    char ch = static_cast<char>(c);
    if (ch == JSON_QUOTE_CHARACTER)
      break;
    if (ch == JSON_ESCAPE_CHARACTER) {
      _cursor.advance(); // consomme '\'
      int esc = _cursor.peek();
      if (esc < 0)
        return ParseValueResult::NO_RESULT;
      val_buf[n++] = static_cast<char>(esc);
      _cursor.advance();
      continue;
    }
    val_buf[n++] = ch;
    _cursor.advance();
  }
  val_buf[n] = '\0';

  if (!cursor_scan_char(_cursor, JSON_QUOTE_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  std::string_view parsed_value(val_buf, n);
  return ParseValueResult::VALUE_PARSED |
         assign_parsed_value_to_value(parsed_value, arg_value);
}

// ── parse_integer ────────────────────────────────────────────
// Extrait les digits dans un buffer local, puis appelle strtol.
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_integer(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_integer\n");
  return parse_numeric<V, int32_t>(arg_value);
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
    JSON_DEBUG_INFO("JSONParserBase::parse_numeric double %fl\n", parsed_value);
  } else if constexpr (std::is_same_v<Type, int32_t>) {
    parsed_value = std::strtol(start, &end, 10);
    JSON_DEBUG_INFO("JSONParserBase::parse_numeric integer %d\n", parsed_value);
  }

  // check if parsed_value have been parsed as Infinity
  if (parsed_value == std::numeric_limits<Type>::infinity()) {
    return parse_infinity(arg_value);
  }

  size_t consumed = static_cast<size_t>(end - start);

  if (consumed == 0)
    return result;

  _cursor.advance(consumed);

  if constexpr (std::is_same_v<Type, int32_t>) {
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
  size_t iterations = 0;
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
    }
    else if (ch == JSON_ESCAPE_CHARACTER && inString) {
      escape = true;
    }
    else if (ch == JSON_QUOTE_CHARACTER) {
      inString = !inString;
    }
    
    if (inString) {
        _cursor.advance();
        continue;
    }

    if (ch == JSON_START_CHARACTER || ch == JSON_ARRAY_START_CHARACTER) {
      depth++;
    }
    else if (ch == JSON_END_CHARACTER || ch == JSON_ARRAY_END_CHARACTER) {
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

  JSON_DEBUG_INFO("JSONParserBase::parse_unknown_value iterations=%zu\n", iterations);

  return ParseValueResult::VALUE_PARSED;
}

// ── parse_value (callback) ────────────────────────────────────
template <typename Cursor>
ParseValueResult JSONParserBase<Cursor>::parse_value(JSONCallbackObject& cb) {
  JSON_DEBUG_WARNING("JSONParserBase<Cursor>::parse_value with callback\n");

  JSONKey parsed_key(_key_start, _key_length);
  cb.setKey(parsed_key);

  if (_is_top_level_array) {
    JSON_DEBUG_INFO("JSONParserBase<Cursor>::parse_value top level array\n");
    return ParseValueResult::KEY_FOUND | parse_array(cb);
  }
  
  //cb.setArrayIndex(_array_index);
  return ParseValueResult::KEY_FOUND | parse_into_value(cb);
}

// ── parse_value (args) ────────────────────────────────────────
template <typename Cursor>
template <class... Args>
enable_if_t<args_are_pairs<Args...>, ParseValueResult>
JSONParserBase<Cursor>::parse_value(Args &&...args) {
  JSON_DEBUG_WARNING("JSONParserBase<Cursor>::parse_value\n");
  JSONKey parsed_key(_key_start, _key_length);
  return searchValueArgumentForKey(0, parsed_key, std::forward<Args>(args)...);
}

// ── parse_into_value ─────────────────────────────────────────
template <typename Cursor>
template <typename V>
ParseValueResult JSONParserBase<Cursor>::parse_into_value(V &arg_value) {
  JSON_DEBUG_TYPES("JSONParserBase<Cursor>::parse_into_value %s\n",
                       arg_value);
  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_same_v<V, bool>) {
    return parse_bool(arg_value);
  } else if constexpr (std::is_floating_point_v<V>) {
    return parse_floating_point(arg_value);
  } else if constexpr (std::is_integral_v<V>) {
    return parse_integer(arg_value);
  } else if constexpr (std::is_same_v<V, std::string_view> ||
                       is_char_array_v<V>) {
    return parse_string(arg_value);
  } else if constexpr (is_uint_array_v<V>) {
    ParseValueResult result = parse_string(arg_value);
    if (result.parsed())
      return result;
    result = parse_array(arg_value);
    if (result.parsed())
      return result;
    return ParseValueResult::NO_RESULT;
  } else if constexpr (is_container_v<V>) {
    return parse_array(arg_value);
  } else if constexpr (std::is_same_v<remove_cvref_t<V>, UnknownValueType>) {
    return parse_any(arg_value);
  } else if constexpr (std::is_base_of_v<JSONData, remove_cvref_t<V>>) {
    return parse_object(arg_value);
  } else if constexpr (std::is_pointer_v<V>) {
    ParseValueResult result = parse_null(arg_value);
    if constexpr (!std::is_const_v<std::remove_pointer_t<V>> &&
                  !std::is_same_v<V, UnknownValueType>) {
      if (!result.parsed() && arg_value != nullptr)
        result |= parse_into_value(*arg_value);
    }
    return result;
  } else {
    return ParseValueResult::NO_RESULT;
  }
}

// template <typename Cursor>
// void JSONParserBase<Cursor>::parse(const JSONCallbackObject& cb) {
   
// }

template <typename Cursor>
template <typename T>
enable_if_t<is_derived_json_data_container_v<T>, void>
JSONParserBase<Cursor>::parse(T& jsonObjects) {
  JSON_DEBUG_INFO("JSONParserBase::parse with derived JSONData objects\n");
  _is_top_level_array = true;
  parse_array(jsonObjects);
  _state = END;
}

// ── parse (boucle principale) ─────────────────────────────────
template <typename Cursor>
template <typename... Args>
void JSONParserBase<Cursor>::parse(Args &&...args) {

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
      }
      break;

    case COLON:
      if (parse_colon()) {
        set_state(VALUE);
      } else {
        _state = ERROR;
      }
      break;

    case VALUE: {
      skip_spaces();
      if (is_object_end()) {
        _state = END;
        continue;
      }
      
      ParseValueResult r = parse_value(std::forward<Args>(args)...);
      
      if (!r.key()) {
        parse_unknown_value();
        set_state(COMMA);
      } else if (r.parsed()) {
        nParsed++;
        set_state(COMMA);
      } else {
        _state = ERROR;
      }
      break;
    }

    case COMMA:
      skip_spaces();
      if (is_object_end()) {
        _state = END;
        continue;
      }
      if (parse_comma()) {
        set_state(KEY);
      } else {
        _state = ERROR;
      }
      break;

    case END:
      _cursor.advance(); // consomme '}' ou ']'
      JSON_DEBUG_INFO(
          "JSONParserBase: parsing complete, iterations=%zu position=%zu\n",
          iteration, get_position());
      return;

    case ERROR:
      JSON_DEBUG_ERROR("JSONParserBase: error at byte %zu\n", get_position());
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

// ── searchValueArgumentForKey ─────────────────────────────────
template <typename Cursor>
inline ParseValueResult
JSONParserBase<Cursor>::searchValueArgumentForKey(size_t /*idx*/,
                                                  JSONKey parsed_key) {
  JSON_DEBUG_INFO("Key '%.*s' not found in parameters\n",
                  (int)parsed_key.length(), parsed_key.data());
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor>
template <typename V, typename... Args>
ParseValueResult JSONParserBase<Cursor>::searchValueArgumentForKey(
    size_t idx, JSONKey parsed_key, JSONKey arg_key, V &arg_value,
    Args &&...args) {
  ParseValueResult result = ParseValueResult::NO_RESULT;

  // if (nKeys >= _nArgs)
  //   return result;

  if (parsed_key == arg_key) {
    JSON_DEBUG_INFO("Found key %.*s", (int)arg_key.length(), arg_key.data());
    JSON_DEBUG_TYPES(" for arg type %s\n", arg_value);
    result |= ParseValueResult::KEY_FOUND | parse_into_value(arg_value);
    nKeys++;
    if (result.updated()) {
      if (_automask)
        keyMask |= (1 << idx);
      else if (arg_key.is_indexed())
        keyMask |= (1 << arg_key.getIndex());
      nUpdated++;
    }
    if (result.converted())
      nConverted++;
    if (_state == STOPPED) {
      _state = END;
      return result;
    }
  } else {
    result = searchValueArgumentForKey(idx + 1, parsed_key,
                                       std::forward<Args>(args)...);
  }
  return result;
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
  V nv = clamp_to_max<PV, V>(pv);
  if (v != nv) {
    v = nv;
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
  if (memcmp(v, pv.data(), pv.length()) == 0)
    return ParseValueResult::NO_RESULT;
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
JSONParserBase<Cursor>::assign_callback_object(PV pv, JSONCallbackObject &cb) {
  cb.run(pv, _array_index);
  if (cb.stop)
    _state = STOPPED;
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
  if (static_cast<From>(std::numeric_limits<To>::max()) < v)
    return std::numeric_limits<To>::max();
  if constexpr (std::is_signed_v<To>) {
    if (static_cast<From>(std::numeric_limits<To>::min()) > v)
      return std::numeric_limits<To>::min();
  // }
  return static_cast<To>(// v);
}

// ── parse_array ────────────────────────────────────────Object───────
// 
template <typename Cursor>
ParseValueResult JSONParserBase<Cursor>::parse_a//  with callback\n");
  JSONCallbackObject cb_obj(cb, JSONKey("$ROOT"));
  return parse_array(cb_obj);
}

// template <typename Cursor>
// template <typename T>
// enable_if_t<is_derived_json_data_container_v<T>, ParseValueResult>
// JSONParserBase<Cursor>::parse_array(T &arg_value) {
//   JSON_DEBUG_INFO("JSONParserBase::parse_array with derived JSONData objects\n");
//   return parse_array(arg_value);
// }

template <typename Cursor>
ParseValueResult JSONParserBase<Cursor>::parse_array(UnknownValueType) {
  std::vector<UnknownValueType> tmp;
  return parse_array(tmp);
}

template <typename Cursor>
template <typename V>
enable_if_t<container_info<V>::is_container || std::is_same_v<JSONCallbackObject, remove_cvref_t<V>>, ParseValueResult>
JSONParserBase<Cursor>::parse_array(V &arg_value) {
  JSON_DEBUG_INFO("JSONParserBase::parse_array\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (!is_array_start()) {
    return result;
  }

  _cursor.advance();

  size_t i = 0;
  while (i < JSON::MAX_ARRAY_LENGTH) {
    skip_spaces();
    result |= parse_into_array_at_index(arg_value, i);
    if (_state == STOPPED)
      return ParseValueResult::VALUE_PARSED;
    if (!result.parsed())
      return ParseValueResult::NO_RESULT;
    skip_spaces();
    if (!cursor_scan_char(_cursor, JSON_COMMA_CHARACTER, true))
      break;
    i++;
  }

  if (!cursor_scan_char(_cursor, JSON_ARRAY_END_CHARACTER, true))
    return ParseValueResult::NO_RESULT;

  skip_spaces();

  return result;
}

template <typename Cursor>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(JSONCallbackObject cb, size_t index) {
  cb.setArrayIndex(index);

  if (_is_top_level_array) {
   JSON_DEBUG_INFO("JSONParserBase::parse_into_array_at_index top level array " "index=%zu\n", index );
    return parse_object(cb);
  }
  
  return parse_into_value(cb);
}

template <typename Cursor>
template <typename T, size_t N>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(T (&array)[N], size_t index) {
  if (index >= N) {
    T dummy;
    return parse_into_value(dummy);
  }

  return parse_into_value(array[index]);
}

template <typename Cursor>
template <typename T>
ParseValueResult
JSONParserBase<Cursor>::parse_into_array_at_index(std::vector<T> &array,
                                                  size_t /*index*/) {
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
                                                  size_t index) {
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
  JSON_DEBUG_INFO("JSONParser::parse_object\n");
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if (!is_object_start()) {
    return ParseValueResult::NO_RESULT;
  }
    
  JSON::ParseResult r = arg_value.fromJSON(_cursor);
    
  if (r.error == true) {
    JSON_DEBUG_TYPES("JSONParser::parse_object error parsing %s\n", arg_value);
    _state = ERROR;
    return ParseValueResult::NO_RESULT;
  }

  if constexpr (std::is_same_v<remove_cvref_t<V>, JSONCallbackObject>) {
    if (r.stopped) {
      JSON_DEBUG_INFO("JSONParser::parse_object parsing stopped\n");
      _state = STOPPED;
    }
  }

#if JSON_DEBUG_LEVEL > 0
  JSON_DEBUG_INFO("JSONParser::parse_object result: ");
  r.print();
#endif

  // nKeys += r.nKeys;
  // nParsed += r.nParsed;
  // nConverted += r.nConverted;
  // nUpdated += r.nUpdated;
  //_elapsed += r.elapsed;

//#ifndef ARDUINO
  //_cursor.set_position(r.length);
#ifdef ARDUINO
  if constexpr (!std::is_same_v<remove_cvref_t<Cursor>, StreamCursor>) {
    _cursor.set_position(r.length);
  }
#else
  _cursor.set_position(r.length);
#endif

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
  if constexpr (std::is_same_v<Cursor, const PointerCursorReader>) {
    size_t max_length = JSON::DEBUG_COLUMN_WIDTH;
    size_t length = std::min(_cursor.size(), max_length);

    [[maybe_unused]] size_t col_number = get_position() / length;
    [[maybe_unused]] size_t col_pos = get_position() % length;
    [[maybe_unused]] const char *dots =
        (_cursor.size()) > max_length ? "..." : "";

    const char *color = (_state == ERROR) ? "\x1b[31m" : "\x1b[32m";

    PRINT_FUNC("%.*s %s pos=%zu it=%zu, p=%p\n%s%*c%s\x1b[0m\n", (int)length,
               _cursor.start() + col_number * length, dots, get_position(),
               iteration, this, color, (int)(col_pos + 1), '^',
               get_state_name().data());
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

// ============================================================
//  Alias publics
// ============================================================

// JSONParser : version originale basée sur PointerCursor
// (remplace la classe JSONParser existante — même interface)
using JSONParser = JSONParserBase<const JSON:: PointerCursorReader>;

#ifdef ARDUINO
using JSONStreamParser = JSONParserBase<JSON::StreamCursor>;
#endif
