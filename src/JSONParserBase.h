#pragma once

// JSONParserBase.h

#include <limits>

#include "PointerCursor.h"
#include "StreamCursor.h"
#include "JSONCallbackObject.h"
#include "JSONObject.h"
//#include "Reflection.h"
#include "Dispatch.h"
#include "StringPool.h"
#include "types.h"
#include "utils.h"

#ifdef __GXX_RTTI
#  include "demangled.h"
#endif

#ifndef ARDUINO
#  include "../include/ArduinoCompat.h"
#endif

using namespace std;

template <bool UseMask, typename PV, typename V>
static inline ParseValueResult assign( PV&& pv, V& v ) {
  if constexpr ( UseMask ) {
    if ( v != pv ) {
      v = pv;
      return ParseValueResult::VALUE_UPDATED;
    }
  } else {
    v = pv;
  }

  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask>
class JSONParserBase {
  public:
  enum ParserState : uint8_t {
    START = 0,
    KEY = 1,
    COLON = 2,
    VALUE = 3,
    COMMA = 4,
    END = 5,
    ERROR = 6,
    STOPPED = 7,
    SKIP = 8
  };

  struct Progress {
    MaskType _keyMask = 0;
    uint8_t _nParsed = 0;
    uint8_t _nMatched = 0;
    uint8_t _nConverted = 0;
    uint8_t _nUpdated = 0;
  };

  // ── Constructeur ─
  template <typename T = Cursor>
  explicit JSONParserBase( T& cursor,
                           std::enable_if_t<is_cursor_v<T>>* = nullptr ):
      _cursor( cursor ),
      _state( START ),
      _progress(),
      //_automask( false ),
      _is_top_level_array( false ),
      _lastError( ParserError::NO_ERROR ),
      _lastParseError( 0 ) {
    JSON_DEBUG_COLOR( COLOR_BLUE, "JSONParserBase created\n" );
    GLOBAL_ITERATIONS = 0;
  }

  ~JSONParserBase() {
    // Do not destroy the Cursor because it may be used by a parent parser.
    JSON_DEBUG_WARNING( "JSONParserBase " );
    reset();
    JSON_DEBUG_WARNING( "destroyed\n" );
#ifdef JSON_DEBUG_MEM
    GLOBAL_PARSER_SIZE -= sizeof( *this );
#endif
  }

  // ── API publique (identique à JSONParser) ─────────────────

  template <typename TargetT, typename Arg>
  enable_if_t<!is_derived_json_data_container_v<TargetT>, void>
  parse( Arg& args );

  template <typename TargetT, typename Arg>
  enable_if_t<is_derived_json_data_container_v<TargetT>, void>
  parse( Arg& jsonObjects );

  ParserState state() { return _state; }
  ParserError error() { return _lastError; }
  ParseValueResult parseError() { return _lastParseError; }

  // ── Méthodes d'assignation (identiques à JSONParser) ──────
  // (reprises telles quelles — logique pure, pas d'accès au curseur)

  template <typename PV, typename V>
  ParseValueResult assign_integral_to_integral( PV& pv, V& v );

  template <typename PV, typename V>
  ParseValueResult assign_same_type( PV& pv, V& v );

  template <typename PV, typename V>
  ParseValueResult assign_convertible( PV& pv, V& v );

  template <typename PV, typename V>
  ParseValueResult assign_string_view_to_char_array( PV& pv, V& v );

  template <typename PV, typename V>
  ParseValueResult assign_null_ptr_to_pointer( PV& pv, V& v );

  template <typename PV, typename V>
  ParseValueResult assign_not_handled( PV& pv, V& v );

  template <typename PV, typename V>
  ParseValueResult assign_parsed_value_to_value( PV& pv, V& v );

  template <typename V> ParseValueResult
  assign_string_view_to_unsigned_array( std::string_view& pv, V& v );

  template <typename PV> ParseValueResult
  assign_callback_object( const PV& pv, JSONCallbackObject& cb );

  template <typename PV, typename V>
  ParseValueResult assign_infinity_to_integral( PV& pv, V& v );

  template <class From, class To> constexpr To clamp_to_min_max( From v );

  template <typename TargetT, typename V>
  ParseValueResult parse_into_value( V& arg_value );

  template <typename TargetT, bool MultiPoint> ParseValueResult
  parse_into_array_at_index( JSONCallbackObject& cb, uint32_t index );

  template <typename TargetT, bool MultiPoint, typename C>
  std::enable_if_t<is_container_v<C>, ParseValueResult>
  parse_into_array_at_index( C& array, uint32_t index );

  ParseValueResult parse_array( JSONCallbackObject& arg_value );

  template <typename TargetT, typename V>
  ParseValueResult parse_array( V& arg_value );

  // Accessors
  size_t nParsed() { return _progress._nParsed; }
  size_t nMatched() { return _progress._nMatched; }
  size_t nConverted() { return _progress._nConverted; }
  size_t nUpdated() { return _progress._nUpdated; }
  MaskType keyMask() { return _progress._keyMask; }
  // bool automask() { return _automask; }
  // void setAutomask( bool automask ) { _automask = automask; }
  bool stopped() { return _state == STOPPED; }
  size_t bytesConsumed() { return _cursor.bytesConsumed(); }
  void reset();
  Cursor& cursor() { return _cursor; }

  private:
  Cursor& _cursor;
  ParserState _state;
  Progress _progress;
  //bool _automask;
  bool _is_top_level_array;
  ParserError _lastError;
  ParseValueResult _lastParseError;

  static uint8_t _key_length;
  static char _s_key_buf[MAX_KEY_LENGTH + 1];

  // ── Primitives de lecture via curseur ──────────────────────
  bool scan_char( char c, bool consume = true );

  template <size_t KwN>
  bool scan_keyword( const char ( &keyword )[KwN]);

  template <size_t RN> constexpr bool scan_ranges_once( char ( &ranges )[RN][2],
                                                        bool consume = true );

  template <size_t RN> constexpr bool scan_ranges( char ( &ranges )[RN][2],
                                                   size_t maxLen = 0,
                                                   bool consume = true );

  char _current_char() {
    int c = _cursor.peek();
    return c < 0 ? '\0' : static_cast<char>( c );
  }

  void skip_spaces();
  bool parse_key();

  template <typename TargetT>
  ParseValueResult parse_value( JSONCallbackObject& cb );

  template <typename TargetT, typename TableT>
  ParseValueResult parse_value( TableT& table);

  void _reset_key();
  template <typename TargetT, typename V>
  bool scan_escaped_string( std::string_view& sv );
  template <typename TargetT, typename V> ParseValueResult parse_string( V& v );

  template <typename PV, bool ParseInfinity = true, typename V>
  ParseValueResult parse_numeric_as( V& v );

  ParseValueResult parse_any( JSONCallbackObject& v );
  ParseValueResult parse_any_keyword( JSONCallbackObject& v );

  template <typename V> ParseValueResult parse_bool( V& v );
  template <typename V> ParseValueResult parse_null( V& v );
  template <typename V> ParseValueResult parse_infinity( V& v );
  template <typename V> ParseValueResult parse_object( V& v );

  ParseValueResult skip_value();

  template <typename T, size_t N = 0>
  std::enable_if_t<N == 0 && std::is_integral_v<T> && std::is_unsigned_v<T>,
                   ParseValueResult>
  skip_value();

  template <typename T, size_t N = 0>
  std::enable_if_t<N == 0 && std::is_integral_v<T> && std::is_signed_v<T>,
                   ParseValueResult>
  skip_value();

  template <typename T, size_t N = 0>
  std::enable_if_t<N == 0 && std::is_same_v<T, bool>, ParseValueResult>
  skip_value();

  template <typename T, size_t N = 0>
  std::enable_if_t<N == 0 && std::is_floating_point_v<T>, ParseValueResult>
  skip_value();

  template <typename T, size_t N = 0>
  std::enable_if_t<N == 0 && std::is_same_v<std::string_view, T>,
                   ParseValueResult>
  skip_value();

  template <typename V> ParseValueResult skip_to_array_end();
  template <typename V> ParseValueResult skip_to_array_end_fast();
  ParseValueResult skip_to_object_end();

  bool parse_colon();
  bool parse_comma();
  bool is_object_start() { return _current_char() == JSON_OBJECT_START_CHARACTER; }
  bool is_object_end() { return _current_char() == JSON_OBJECT_END_CHARACTER; }
  bool is_array_start() { return _current_char() == JSON_ARRAY_START_CHARACTER; }
  bool is_array_end() { return _current_char() == JSON_ARRAY_END_CHARACTER; }
  size_t scan_digits( size_t max_length = 0 );
  //size_t is_start_numeric();

  void set_state( ParserState s );
#if JSON_DEBUG_LEVEL > 0
  std::string_view get_state_name();
  void print_state( size_t iteration );
#endif
};
/*
    END OF JSONPARSERBASE DECLARATION
*/
template <typename Cursor, bool UseMask>
uint8_t JSONParserBase<Cursor, UseMask>::_key_length = 0;

template <typename Cursor, bool UseMask>
char JSONParserBase<Cursor, UseMask>::_s_key_buf[JSON::MAX_KEY_LENGTH + 1] = {};

// template <typename Cursor, bool UseMask>
// char JSONParserBase<UseMask>::_s_val_buf[JSON::MAX_VALUE_LENGTH + 1] = {};

// ============================================================
//  Implémentation des méthodes
// ============================================================

template <typename Cursor, bool UseMask>
void JSONParserBase<Cursor, UseMask>::reset() {
  // _cursor is passed from parser to parser and should not be reset
  //  _bytesConsumed = 0;
  _state = START;
  _progress = Progress();
  //_automask = false;
  _is_top_level_array = false;
  //  _lastError = ParserError::NO_ERROR;
  //  _lastParseError = ParseValueResult();
  _key_length = 0;
}
// ============================================================
//
//                    PARSING
//
// ============================================================

template <typename Cursor, bool UseMask>
template <typename TargetT, typename Arg>
enable_if_t<is_derived_json_data_container_v<TargetT>, void>
JSONParserBase<Cursor, UseMask>::parse( Arg& jsonObjects ) {
  JSON_DEBUG_INFO( "JSONParserBase::parse with derived JSONObject objects\n" );
  _is_top_level_array = true;
  parse_array<TargetT>( jsonObjects );
  _state = END;
}

// ── parse (boucle principale) ─────────────────────────────────
template <typename Cursor, bool UseMask>
template <typename TargetT, typename Arg>
enable_if_t<!is_derived_json_data_container_v<TargetT>, void>
JSONParserBase<Cursor, UseMask>::parse( Arg& arg ) {

  _is_top_level_array = false;

  while ( !_cursor.eof() ) {
    CHECK_LOOP( MAX_ITERATIONS, _state = ERROR; _lastError = ParserError::TOO_MANY_ITERATIONS; return; )

#if JSON_DEBUG_LEVEL == 1
    print_state( GLOBAL_ITERATIONS );
#endif
    switch ( _state ) {
      case START:
        SKIP_SPACES

        if ( is_array_start() ) {
          _is_top_level_array = true;
          _state = VALUE;
          continue;
        }

        if ( is_object_start() ) {
          _cursor.advance();
          _state = KEY;
        } else {
          _state = ERROR;
          _lastError = ParserError::NO_OBJECT_START;
        }

        break;
      case KEY:
        SKIP_SPACES
        if ( is_object_end() ) {
          _state = END;
          //_cursor.advance();
          continue;
        } else if ( parse_key() ) {
          _progress._nParsed++;
          JSON_DEBUG_INFO( "nParsed=%zu\n", _progress._nParsed );
          set_state( COLON );
        } else {
          _state = ERROR;
          _lastError = ParserError::INVALID_KEY;
        }

        break;
      case COLON:
        if ( parse_colon() ) {
          set_state( VALUE );
        } else {
          _state = ERROR;
          _lastError = ParserError::NO_COLON;
        }
        
        break;
      case VALUE: {

        SKIP_SPACES
        if ( is_object_end() ) {
          _state = END;
          //_cursor.advance();
          continue;
        }

        ParseValueResult r = parse_value<TargetT>(arg);

        _progress._nMatched += r.keyFound() ? 1 : 0;

        if ( !r.keyFound() ) { // The key was not found in the arguments. This
                               // is not an error. We skip the value.
          r = skip_value();
        }

        if ( r ) {
          set_state( COMMA );
        } else { // The key was found but the value was not parsed. This is an
                 // error.
          _state = ERROR;
          _lastError = ParserError::INVALID_VALUE;
          _lastParseError = r;
        }

        if constexpr ( is_dispatch_info_v<Arg> ) {
          if ( _progress._nMatched >= std::tuple_size<Arg>::value ) {
            JSON_DEBUG_WARNING(
                "JSONParserBase::parse: Parser depth %zu: "
                "all keys found,(%zu/%zu) skiping to object end\n",
                _cursor.depth,
                _progress._nMatched,
                std::tuple_size<Arg>::value );
            _state = SKIP;
            break;
          }
        }
      }

      break;
      case COMMA:
        SKIP_SPACES
        if ( is_object_end() ) {
          //_state = END;
          _cursor.advance();
          return;
        }
        if ( parse_comma() ) {
          set_state( KEY );
        } else {
          _state = ERROR;
          _lastError = ParserError::NO_COMMA;
        }

        break;
      case END:
        _cursor.advance();
#if JSON_DEBUG_LEVEL > 0
        if ( _cursor.depth == 0 ) {
          JSON_DEBUG_INFO(
              "JSONParserBase: parsing complete, iterations=%zu position=%zu\n",
              GLOBAL_ITERATIONS,
              _cursor.bytesConsumed() );
        }
#endif
      return;
      case ERROR:
        JSON_DEBUG_ERROR( "JSONParserBase: error at byte %zu: %s\n", _cursor.bytesConsumed(), errorToString( _lastError ) );
#if JSON_DEBUG_LEVEL > 0
        print_state( GLOBAL_ITERATIONS );
#endif
        return;
      case STOPPED:
        if constexpr ( is_callback<TargetT> ) {
          JSON_DEBUG_INFO( "JSONParserBase: stopped by callback\n" );
          return;
        } else {
          ParseValueResult r = skip_to_object_end();

          if ( r ) {
            _state = END;
          } else {
            _state = ERROR;
            _lastError = ParserError::INVALID_VALUE;
            _lastParseError = r;
          }
          break;
        }
      case SKIP: {
        JSON_DEBUG_INFO( "JSONParserBase: skip\n" );
        ParseValueResult r = skip_to_object_end();
        if ( r ) {
          _state = END;
        } else {
          _lastError = ParserError::SKIP_ERROR;
          _lastParseError = r;
          _state = ERROR;
        }
      }

        break;
      default:
        return;
    }
  }
}

// ── parse_key ────────────────────────────────────────────────
template <typename Cursor, bool UseMask>
bool JSONParserBase<Cursor, UseMask>::parse_key() {
  if ( !scan_char( JSON_QUOTE_CHARACTER, true ) ) {
    _reset_key();
    return false;
  }

  // Pour StreamCursor on doit copier la clé dans un buffer local.
  // Pour PointerCursor on peut pointer directement (comportement original).
  // On utilise un buffer statique court pour la clé.
  uint16_t n = 0;

  while ( n < MAX_KEY_LENGTH ) {
    int c = _cursor.peek();

    if ( c < 0 ) {
      _reset_key();
      return false;
    }
    
    char ch = static_cast<char>( c );
    // Valide si dans JSON_KEY_CHARACTERS (ranges a–z A–Z 0–9 _ $)
    //bool valid = is_in_mask(c, ALPHA_LOWER_CHARACTERS_MASK, 'a') || is_in_mask(c, ALPHA_UPPER_CHARACTERS_MASK, 'A') || is_in_mask(c, _$_CHARACTERS_MASK, '$' );
    if ( !is_in_ranges(ch, JSON_KEY_CHARACTERS_RANGES) ) break;

    _s_key_buf[n++] = ch;
    _cursor.advance();
  }

  if ( n == 0 ) { return false; }

  //_cursor.advance( n ); // consomme les caractères de la clé

  if ( _cursor.peek() != JSON_QUOTE_CHARACTER) {
    _reset_key();
    return false;
  }

  _cursor.advance();

  _key_length = n;

  JSON_DEBUG_INFO(
      "JSONParserBase::parse_key '%.*s'\n", (int)_key_length, _s_key_buf );
  return true;
}

// ── parse_colon ───────────────────────────────────────────────
template <typename Cursor, bool UseMask>
bool JSONParserBase<Cursor, UseMask>::parse_colon() {
  SKIP_SPACES
  return scan_char( JSON_COLON_CHARACTER, true );
}

// ── parse_comma ───────────────────────────────────────────────
template <typename Cursor, bool UseMask>
bool JSONParserBase<Cursor, UseMask>::parse_comma() {
  return scan_char( JSON_COMMA_CHARACTER, true );
}

// ── parse_value (callback) ────────────────────────────────────
template <typename Cursor, bool UseMask> template <typename TargetT>
ParseValueResult
JSONParserBase<Cursor, UseMask>::parse_value( JSONCallbackObject& cb ) {
  JSON_DEBUG_WARNING("JSONParserBase<Cursor, UseMask>::parse_value with callback %c\n", _cursor.peek());

  cb.setKey( _s_key_buf, _key_length );

  if ( _is_top_level_array ) {
    JSON_DEBUG_INFO("JSONParserBase<Cursor, UseMask>::parse_value top level array\n");
    return parse_array( cb ) | ParseValueResult::KEY_FOUND;
  }

  return parse_into_value<TargetT>( cb ) | ParseValueResult::KEY_FOUND;
}

template <typename Cursor, bool UseMask>
template <typename TargetT, typename TableT>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_value( TableT& table ) {

  std::string_view parsed_key( _s_key_buf, _key_length );
  //auto entry = find_entry( table, parsed_key );

  ParseValueResult result = ParseValueResult::NO_RESULT;
  
  // std::visit( [&]( auto&& arg ) {
  //   using T = std::decay_t<decltype(arg)>;
  //   if constexpr ( !std::is_same_v<T, std::monostate> ) {
  //     result |= ParseValueResult::KEY_FOUND;
  //     result |= parse_into_value<TargetT>( arg.get() );
  //   }
  // }, entry.variant);
  size_t key_index = 0;

  dispatch_by_key( table, parsed_key, [&]( auto&& arg, size_t index ) {
    result |= ParseValueResult::KEY_FOUND;
    result |= parse_into_value<TargetT>( arg );
    key_index = index;
  });  

  if (!result.keyFound()) {
    JSON_DEBUG_WARNING( "JSONParserBase<Cursor, UseMask>::parse_value "
    "key '%.*s' not found\n",
    (int)parsed_key.length(),
    parsed_key.data() );
    
    return result;
  }

  if ( result.updated() ) {
    if constexpr ( UseMask ) {

//      if (_automask) {
        _progress._keyMask |= ( 1u << static_cast<MaskType>(key_index) );
      // } else if (entry.mask_index >= 0) {
      //   _progress._keyMask |= ( 1u << static_cast<MaskType>(entry.mask_index) );
      // }
    }

    _progress._nUpdated++;
  }

  if ( result.converted() ) _progress._nConverted++;

  return result;
}

// ── parse_into_value ─────────────────────────────────────────
template <typename Cursor, bool UseMask> template <typename TargetT, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::parse_into_value( V& arg_value ) {
  JSON_DEBUG_TYPES( "JSONParserBase<Cursor, UseMask>::parse_into_value %s\n", arg_value );
  // LOG_STACK("parse_into_value");

  if constexpr ( is_callback<V> ) {
    return parse_any( arg_value );
  } else if constexpr ( std::is_same_v<V, bool> ) {
    return parse_bool( arg_value );
  } else if constexpr ( std::is_floating_point_v<V> ) {
    return parse_numeric_as<V>( arg_value );
  } else if constexpr ( std::is_integral_v<V> ) {
    return parse_numeric_as<V>( arg_value );
  } else if constexpr ( std::is_same_v<V, std::string_view> ||
                        is_char_array_v<V> ) {
    return parse_string<TargetT>( arg_value );
  } else if constexpr ( is_uint_array_v<V> ) {
    ParseValueResult result = parse_string<TargetT>( arg_value );

    if ( result ) { return result; }

    return parse_array<TargetT>( arg_value );
  } else if constexpr ( is_container_v<V> ) {
    return parse_array<TargetT>( arg_value );
  } else if constexpr ( std::is_base_of_v<JSONObject, remove_cv_ref_t<V>> ) {
    return parse_object( arg_value );
  } else if constexpr ( std::is_pointer_v<V> ) {
    ParseValueResult result = parse_null( arg_value );
    if constexpr ( !std::is_const_v<std::remove_pointer_t<V>> ) {
      if ( !result.parsed() && arg_value != nullptr )
        result = parse_into_value<TargetT>( *arg_value );
    }
    return result;
  } else {
    return ParseValueResult::PARSE_ERROR_UNKNOWN;
  }
}

template <typename Cursor, typename TargetV>
static bool needs_pool( bool unescaped ) {
  // JSON_DEBUG_COLOR(COLOR_MAGENTA, "needs_pool<%s, %s>(%d)\n",
  // typeid(Cursor).name(), typeid(TargetT).name(), unescaped);

  // Quand avons-nous besoin d'un pool ?
  // 1. Quand le string est échappé (unescaped == true) dans tous les cas
  // 2. Quand le curseur est un StreamCursor et que la cible est une string_view

  if constexpr ( is_stream_cursor_reader_v<Cursor> &&
                 std::is_same_v<remove_cv_ref_t<TargetV>, std::string_view> ) {
    return true;
  } else {
    return unescaped;
  }
}

template <typename Cursor, bool UseMask>
template <typename TargetT, typename V> bool
JSONParserBase<Cursor, UseMask>::scan_escaped_string( std::string_view& sv ) {
  using Pool = StringPool<TargetT>;

  bool inEscape = false;
  bool unescaped = false;
  int n = 0;
  Pool::ensure_pool_size( 1 );
  char* pool_start_ptr = Pool::current_pos();

  while ( true ) {
    CHECK_LOOP( MAX_ITERATIONS, return false; );

    int c = _cursor.peek();

    if ( c < 0 ) { break; }

    char ch = static_cast<char>( c );

    if ( inEscape ) {
      inEscape = false;
      if ( ch != JSON_QUOTE_CHARACTER ) {
        Pool::write_at( ch, n );
      } else {
        unescaped = true;
      }
    } else {
      if ( ch == JSON_ESCAPE_CHARACTER ) {
        inEscape = true;
        _cursor.advance();
        continue;
      } else if ( ch == JSON_QUOTE_CHARACTER ) {
        break;
      }
    }

    Pool::write_at( ch, n );

    _cursor.advance();
    n++;
  }

  if ( n >= JSON::MAX_VALUE_LENGTH ) { return false; }

  // Le pool est alloué à l'avance uniquement pour les StreamCursor avec des
  // valeurs std::string_view dans JSONParser _parse_impl<true, StreamCursor,
  // UserStruct>
  if ( needs_pool<Cursor, V>( unescaped ) ) {
    Pool::increment_values_counter();
    Pool::move_offset( n );
  }

  sv = std::string_view( pool_start_ptr, n );

  return true;
}

// For the PointerCursorReader, we scan the string as we would normally do for
// non escaped strings. Then we peek back by one and check if the previous
// character was an escape character. If it was, we go back to the start of the
// string and scan it again, this time handling escape sequences. Then we
// advance the cursor by the length of the string.
// Au lieu de deux spécialisations totales de parse_string,
// une seule implémentation dans le template primaire :

template <typename Cursor, bool UseMask> template <typename TargetT, typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_string( V& arg_value ) {
  if ( !scan_char( JSON_QUOTE_CHARACTER, true ) ) {
    return ParseValueResult::PARSE_ERROR_STRING_NO_START;
  }
    
  std::string_view parsed_value;
  // Plusieurs cas: Cursor = PointerCursorReader ou StreamCursor
  // TargetT = Cursor si appel user depuis JSON::parse, UserStruct si appel user
  // depuis fromJSON, JSONCallbackObject si appel depuis parse avec callback
  if constexpr ( is_pointer_cursor_reader_v<Cursor> ) {
    // Chemin rapide : pointer direct dans le buffer

    const char* ptr = _cursor.ptr();
    size_t len = 0;
    size_t max_length = _cursor.available() - 1;
    
    while (len < max_length) {
      if (*(ptr + len) == JSON_QUOTE_CHARACTER) break;
      len++;
    }

    if (len == max_length) {
      return ParseValueResult::PARSE_ERROR_STRING_NO_END;
    }

    parsed_value = std::string_view( ptr, len );
    // Si le caractère précédent est un caractère d'échappement, on doit
    // réanalyser la chaîne avec les caractères d'échappement.
    if ( *(ptr + len - 1) == JSON_ESCAPE_CHARACTER ) {
      _cursor.go_to( ptr );

      if ( !scan_escaped_string<TargetT, V>( parsed_value ) )
        return ParseValueResult::PARSE_ERROR_STRING_ESCAPE;
    } else {
      _cursor.advance( len );
    }
  } else if constexpr ( is_stream_cursor_reader_v<Cursor> ) {
    // StreamCursor : pool obligatoire
    if ( !scan_escaped_string<TargetT, V>( parsed_value ) )
      return ParseValueResult::PARSE_ERROR_STRING_ESCAPE;
  }

  if ( _cursor.read() != JSON_QUOTE_CHARACTER )
    return ParseValueResult::PARSE_ERROR_STRING_NO_END;

  return assign_parsed_value_to_value( parsed_value, arg_value ) |
         ParseValueResult::STRING_PARSED;
}

template <typename Cursor, bool UseMask>
template <typename PV, bool ParseInfinity, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::parse_numeric_as( V& arg_value ) {
  JSON_DEBUG_INFO( "JSONParserBase::parse_numeric\n" );
  JSON_DEBUG_WARNING("1 current position %c\n", _cursor.peek());
  if (_cursor.available() < 0) return ParseValueResult::PARSE_ERROR_NUMERIC;

  // 1. Gestion du signe
  bool negative = false;
  char sign = _cursor.peek();
  if (sign == '-') {
      negative = true;
      _cursor.advance();
  } else if (sign == '+') {
    _cursor.advance();
  }

  uint64_t value = 0;
  int8_t decimal_digits = -1;
  bool has_digits = false;

  // 2. Parser la partie entière et décimale en un seul grand entier
  
  while (_cursor.available() >= 0) {
      char digit = _cursor.peek();
      if (digit >= '0' && digit <= '9') {
          value = value * 10 + (digit - '0');
          has_digits = true;
          if (decimal_digits >= 0) {
              decimal_digits++;
          }
        _cursor.advance();
      } else if (digit == '.') {
          if (decimal_digits >= 0) break; // Deuxième point trouvé (erreur de syntaxe)
          decimal_digits = 0;
        _cursor.advance();
      } else {
          break; // Caractère non numérique ou début de l'exposant 'e'/'E'
      }
  }

  if (!has_digits) {
    if constexpr(ParseInfinity && std::is_floating_point_v<PV>) {
      JSON_DEBUG_WARNING("2 current position %c\n", _cursor.peek());
      return parse_infinity(arg_value);
    }
    else {
      return ParseValueResult::PARSE_ERROR_NUMERIC;
    }
  }

  // Si PV est un entier on peut ignorer la suite jusqu'à l'assignement
  if constexpr(std::is_integral_v<PV> ) {
    if constexpr (ALLOW_PARSING_INTEGER_AS_FLOAT == false) {
      if (decimal_digits > 0) return ParseValueResult::PARSE_ERROR_NUMERIC;
    }
    
    int64_t parsed_value = negative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value);
    if (decimal_digits > 0) parsed_value = multiplyByPowerOfTen(parsed_value, -decimal_digits);
    // les overflows sont gérés dans assign_integral_to_integral
    return assign_parsed_value_to_value( parsed_value, arg_value ) | ParseValueResult::INTEGER_PARSED;
  }
  // Ajustement de l'exposant de base lié à la virgule
  int exponent = (decimal_digits > 0) ? -decimal_digits : 0;

  // 3. Gestion de la notation scientifique JSON (e ou E)
  if (_cursor.available() >= 0) {
      char exp_char = _cursor.peek();
      if (exp_char == 'e' || exp_char == 'E') {
        _cursor.advance();

          bool exp_negative = false;
          char exp_sign = _cursor.peek();
          if (_cursor.available() >= 0) {
              if (exp_sign == '-') { exp_negative = true; _cursor.advance(); }
              else if (exp_sign == '+') { _cursor.advance(); }
          }

          int exp_value = 0;
          while (_cursor.available() >= 0) {
              char digit = _cursor.peek();
              if (digit >= '0' && digit <= '9') {
                  exp_value = exp_value * 10 + (digit - '0');
                _cursor.advance();
              } else {
                  break;
              }
          }
          exponent += exp_negative ? -exp_value : exp_value;
      }
  }

  // 4. Conversion finale (Calcul à la volée pour économiser la Flash ROM)
  double result = multiplyByPowerOfTen(value, exponent);

  if (negative) result *= -1;
  
  return assign_parsed_value_to_value( result, arg_value ) | ParseValueResult::FLOAT_PARSED;
}

// ── parse_bool ───────────────────────────────────────────────
template <typename Cursor, bool UseMask> template <typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_bool( V& arg_value ) {
  JSON_DEBUG_INFO( "JSONParserBase::parse_bool\n" );
  if ( scan_keyword( JSON_FALSE ) ) {
    bool pv = false;
    return assign_parsed_value_to_value( pv, arg_value ) |
           ParseValueResult::BOOLEAN_PARSED;
  }

  if ( scan_keyword( JSON_TRUE ) ) {
    bool pv = true;
    return assign_parsed_value_to_value( pv, arg_value ) |
           ParseValueResult::BOOLEAN_PARSED;
  }

  return ParseValueResult::PARSE_ERROR_BOOLEAN;
}

// ── parse_null ───────────────────────────────────────────────
template <typename Cursor, bool UseMask> template <typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_null( V& arg_value ) {
  JSON_DEBUG_INFO( "JSONParserBase::parse_null\n" );
  if ( !scan_keyword( JSON_NULL ) ) {
    return ParseValueResult::PARSE_ERROR_NULL;
  }

  NullType pv;
  return assign_parsed_value_to_value( pv, arg_value ) |
         ParseValueResult::NULL_VALUE_PARSED;
}

// ── parse_infinity ───────────────────────────────────────────
template <typename Cursor, bool UseMask> template <typename V> ParseValueResult
JSONParserBase<Cursor, UseMask>::parse_infinity( V& arg_value ) {
  JSON_DEBUG_WARNING( "JSONParserBase::parse_infinity\n" );
  if ((static_cast<unsigned char>(_cursor.peek()) & 72) != 72) return ParseValueResult::PARSE_ERROR_NUMERIC;

  if ( scan_keyword( JSON_INFINITY ) ) {
    InfinityType pv;
    return assign_parsed_value_to_value( pv, arg_value ) | ParseValueResult::FLOAT_PARSED;
  } else if (scan_keyword( JSON_NAN )) {
    NullType pv;
    return assign_parsed_value_to_value( pv, arg_value ) | ParseValueResult::FLOAT_PARSED;
  }
    
  return ParseValueResult::PARSE_ERROR_NUMERIC;
}

template <typename Cursor, bool UseMask> ParseValueResult
JSONParserBase<Cursor, UseMask>::parse_array( JSONCallbackObject& arg_value ) {
  arg_value.push();
  ParseValueResult r = parse_array<JSONCallbackObject>( arg_value );
  arg_value.pop();

  return r;
}
/*
template <size_t D, typename T>
constexpr auto& get_element_at_path(T& container, const uint8_t* path) {
  if constexpr (container_info<T>::is_container && container_info<T>::dimensions
> D) { using Elem = typename container_info<T>::child_t; uint8_t index =
path[0]; if constexpr(container_info<T>::kind == ContainerKind::STD_VECTOR) { if
(index >= container.size()) { container.resize(index + 1);
        //JSON_DEBUG_ERROR("Index %d out of bounds for container of size %d\n",
index, container.size());
      }
    }
    return get_element_at_path<D, Elem>(container[index], path + 1);
  } else {
    return container; // base_type atteint (ou char[N] traité comme feuille)
  }
}

template <typename T>
T& get_vector_element(std::vector<T>& container, uint8_t index) {
  if (index >= container.size()) {
    T value{};
    container.push_back(value);
  }

  return container[index];
}
*/
/*
The following function is an optimization for parsing the geometry field in a
geojson file. The geometry field is a nested containers
(container_info<V>::is_container == true) structure with a leaf type. The goal
is to parse the geometry field in a single pass without recursion.

template <typename Cursor, bool UseMask>
template <typename TargetT, typename V>
enable_if_t<(container_info<V>::dimensions > 1U), ParseValueResult>
JSONParserBase<Cursor, UseMask>::parse_array(V& arg_value) {
  constexpr size_t D = container_info<V>::dimensions;
  using BaseContainerType = typename container_info<V>::base_container_t;
  uint8_t path[D] = {0};
  uint8_t depth = 0;

  JSON_DEBUG_INFO("JSONParserBase::parse_array multi-dimensional\n");

  if (!is_array_start()) {
    return ParseValueResult::PARSE_ERROR_ARRAY_NO_START;
  }

  while (true) {
    CHECK_LOOP(1000, ParseValueResult::PARSE_ERROR_OVERFLOW);

    if (depth == D - 1) {
      BaseContainerType& base_container = get_element_at_path<1>(arg_value,
path); ParseValueResult result = parse_array<BaseContainerType>(base_container);
           JSON_DEBUG_INFO("JSONParserBase::parse_array multi-dimensional depth
%d index %d result %s\n", depth, path[depth], errorToString(result));

           if (!result.parsed()) {
              return result;
           }

           if (is_array_end()) {
              depth--;
              path[depth]++;
              _cursor.advance();
              if (depth == 0) {
                return ParseValueResult::ARRAY_PARSED;
              }
              continue;
           }

            SKIP_SPACES
            if (!scan_char(JSON_COMMA_CHARACTER, true)) {
              return ParseValueResult::PARSE_ERROR_ARRAY_NO_COMMA;
            }
            path[depth-1]++;
            SKIP_SPACES
            continue;
    } else {
      if (is_array_end()) {
        JSON_DEBUG_INFO("JSONParserBase::parse_array multi-dimensional ] depth
%d\n", depth); depth--; path[depth]++; _cursor.advance();

        if (depth == 0) {
          return ParseValueResult::ARRAY_PARSED;
        }

      } else if (is_array_start()) {
        JSON_DEBUG_INFO("JSONParserBase::parse_array multi-dimensional [ depth
%d\n", depth); depth++; path[depth] = 0; _cursor.advance();
      }
    }

    JSON_DEBUG_INFO("JSONParserBase::parse_array multi-dimensional loop %d\n",
depth); SKIP_SPACES
  }

  return ParseValueResult::PARSE_ERROR_ARRAY_NO_END;
}
*/
template <typename Cursor, bool UseMask> template <typename TargetT, typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_array( V& arg_value ) {
  JSON_DEBUG_INFO( "JSONParserBase::parse_array\n" );
  LOG_STACK( "parse_array" );

  if ( !is_array_start() ) {
    return ParseValueResult::PARSE_ERROR_ARRAY_NO_START;
  }

  _cursor.advance();

  int i = 0;
  bool underflow = false;
  constexpr int max_array_length = static_cast<int>( MAX_ARRAY_LENGTH );
  constexpr int max = container_info<V>::is_container
                          ? static_cast<int>( container_info<V>::extent )
                          : max_array_length;

  constexpr bool is_multipoint = container_info<V>::is_container &&
                                 container_info<V>::dimensions == 2 &&
                                 is_coords<typename container_info<V>::base_container_t>;
  while ( i < max ) {

    if ( i > max_array_length ) {
      JSON_DEBUG_WARNING( "JSONParserBase::parse_array: too many elements\n" );
      _state = ERROR;
      _lastError = ParserError::TOO_MANY_ITERATIONS;
      return ParseValueResult::PARSE_ERROR_OVERFLOW;
    }

    SKIP_SPACES

    ParseValueResult result = parse_into_array_at_index<TargetT, is_multipoint>( arg_value, i );

    if constexpr ( is_callback<V> ) {
      if ( _state == STOPPED ) {
        return ParseValueResult::ARRAY_PARSED;
      } else if ( _state == SKIP ) {
        return skip_to_array_end<V>();
      }
    }

    if ( !result ) {
      JSON_DEBUG_WARNING(
          "JSONParserBase::parse_array: cannot parse value at index %zu\n", i );
      return result;
    }

    SKIP_SPACES

    if ( !scan_char( JSON_COMMA_CHARACTER, true ) ) {
      JSON_DEBUG_WARNING( "JSONParserBase::parse_array: no comma after index %zu, assuming end of array\n", i );
      if constexpr ( container_info<V>::is_container ) {
        if ( i < max - 1 ) { underflow = true; }
      }

      break;
    }

    i++;
  }

  if ( !is_array_end() ) {
    if ( underflow ) {
      JSON_DEBUG_WARNING( "JSONParserBase::parse_array: no array end\n" );
      _state = ERROR;
      return ParseValueResult::PARSE_ERROR_ARRAY_NO_END;
    } else {
      return skip_to_array_end<V>();
    }
  }

  _cursor.advance();

  SKIP_SPACES

  return ParseValueResult::ARRAY_PARSED;
}

template <typename Cursor, bool UseMask>
template <typename TargetT, bool MultiPoint>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_into_array_at_index( JSONCallbackObject& cb, uint32_t index ) {
  cb.setArrayIndex( index );

  if ( _is_top_level_array ) { return parse_object( cb ); }

  return parse_into_value<TargetT>( cb );
}

template <typename Cursor, bool UseMask>
template <typename TargetT, bool MultiPoint, typename C>
std::enable_if_t<is_container_v<C>, ParseValueResult>
JSONParserBase<Cursor, UseMask>::parse_into_array_at_index( C& array, uint32_t index ) {
  using ContainerInfo = container_info<C>;

  if constexpr ( ContainerInfo::fixed ) {
    if ( index >= ContainerInfo::extent ) {
      JSON_DEBUG_COLOR( COLOR_RED, "Array overflow at index %zu\n", index );
      return ParseValueResult::PARSE_ERROR_OVERFLOW;
    }
  }

  if constexpr ( ContainerInfo::kind == ContainerKind::STD_VECTOR || ContainerInfo::kind == ContainerKind::STD_LIST ) {
// Avoid memory fragmentation on Arduino by reserving memory in chunks of 100
#ifdef ARDUINO
    if constexpr ( MultiPoint ) {
      if ( ((index + 1) % 100) == 0 ) { 
        array.reserve( index + 100 );
      }
    }
#endif
    if ( index == array.size()) {
      array.emplace_back();
    }
  }

#if defined(__GXX_RTTI) && JSON_DEBUG_LEVEL > 0
  using ElementType = typename ContainerInfo::child_t;
  JSON_DEBUG_COLOR( COLOR_GREEN,
                    "Parsing array at index %zu into %s\n",
                    index,
                    typeid( ElementType ).name() );
#endif
  
  return parse_into_value<TargetT>( array[index] );
}

// template <typename Cursor, bool UseMask>
// template <typename TargetT, typename C>
// std::enable_if_t<is_c_array_v<C>, ParseValueResult>
// JSONParserBase<Cursor, UseMask>::parse_into_array_at_index( C&array, uint32_t index ) {
//   constexpr size_t N = container_info<C>::extent;

//   if ( index >= N ) {
//     JSON_DEBUG_WARNING(
//         "JSONParserBase::parse_into_array_at_index: %zu overflow", index );
//     return ParseValueResult::PARSE_ERROR_OVERFLOW;
//   }

//   return parse_into_value<TargetT>( array[index] );
// }

// template <typename Cursor, bool UseMask>
// template <typename TargetT, typename C>
// std::enable_if_t<is_vector_v<C>, ParseValueResult>
// JSONParserBase<Cursor, UseMask>::parse_into_array_at_index( C& array, uint32_t index ) {
//   using T = typename container_info<C>::base_t;

//   if constexpr ( is_coords<T> ) {
//     if ( _is_geometry && index % 100 == 0 ) { array.reserve( index + 100 ); }
//   }

//   array.emplace_back();
//   return parse_into_value<TargetT>( array[index] );
// }

// ── parse_object ──────────────────────────────────────────────
template <typename Cursor, bool UseMask> template <typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_object( V& arg_value ) {
  JSON_DEBUG_TYPES( "JSONParser::parse_object into %s\n", arg_value );
  LOG_STACK( "parse_object" );

  if ( !is_object_start() ) {
    return ParseValueResult::PARSE_ERROR_OBJECT_NO_START;
  }

  JSON_DEBUG_INFO(
      "Will parse object for key '%.*s' Cursor position is now at %zu\n",
      _key_length,
      _s_key_buf,
      _cursor.bytesConsumed() );

  if constexpr ( is_callback<V> ) { arg_value.push(); }

  Progress progress = _progress;

  reset();
  _cursor.depth++;
  JSON::ParseResult r = arg_value.fromJSON( this );
  _cursor.depth--;

  _progress = progress;

  if constexpr ( is_callback<V> ) { arg_value.pop(); }

#if JSON_DEBUG_LEVEL > 0
  JSON_DEBUG_INFO( "In previous JSONParser, parse_object result: " );
  r.print();
  JSON_DEBUG_INFO( "Cursor position is now at %zu\n", _cursor.bytesConsumed() );
#endif

  if ( r.error != NO_ERROR ) {
    JSON_DEBUG_TYPES( "In previous JSONParser::parse_object error parsing %s :",
                      arg_value );
    JSON_DEBUG_INFO( "%s\n", errorToString( r.error ) );
    _state = END;
    _lastError = r.error;
    _lastParseError = r.parseError;
    return _lastParseError;
  }

  if constexpr ( is_callback<V> ) {
    if ( r.stopped ) {
      JSON_DEBUG_INFO( "JSONParser::parse_object parsing stopped\n" );
      _state = STOPPED;
    }
  }

  return ParseValueResult::OBJECT_PARSED | ParseValueResult::VALUE_CONVERTED;
}

template <typename Cursor, bool UseMask>
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_any_keyword( JSONCallbackObject& arg_value ) {
  ParseValueResult result = ParseValueResult::NO_RESULT;
  unsigned char ch = static_cast<unsigned char>(_cursor.peek());

  if (ch < 'I' || ch > 't') return result;

  if ((result = parse_bool( arg_value ))) return result;
  if ((result = parse_null( arg_value ))) return result;
  if ((result = parse_infinity( arg_value ))) return result;

  return result;
}
// ── parse_any ─────────────────────────────────────────────────
template <typename Cursor, bool UseMask> 
ParseValueResult JSONParserBase<Cursor, UseMask>::parse_any( JSONCallbackObject& arg_value ) {
  ParseValueResult result = ParseValueResult::NO_RESULT;

  if ( (result = parse_string<JSONCallbackObject>( arg_value ))) return result;
  if ( (result = parse_numeric_as<double, false>( arg_value )) ) return result;
  if ( (result = parse_any_keyword( arg_value )) ) return result;
  if ( is_array_start() ) return parse_array( arg_value );
  if ( is_object_start() ) return parse_object( arg_value );

  return ParseValueResult::NO_RESULT;
}

// ============================================================
//
//                    SKIPPING
//
// ============================================================

// ── skip_value ───────────────────────────────────────
// Saute une valeur JSON sans la parser (objet, tableau, littéral...)
template <typename Cursor, bool UseMask>
ParseValueResult JSONParserBase<Cursor, UseMask>::skip_value() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_value generic\n" );

  int8_t depth = 0;
  bool inString = false;
  bool escape = false;

  while ( true ) {
    CHECK_LOOP( MAX_ITERATIONS, return ParseValueResult::PARSE_ERROR_OVERFLOW; );

    int c = _cursor.peek();

    if ( c < 0 ) return ParseValueResult::PARSE_ERROR_UNKNOWN;

    char ch = static_cast<char>( c );

    if ( escape ) {
      escape = false;
    } else if ( ch == JSON_ESCAPE_CHARACTER && inString ) {
      escape = true;
    } else if ( ch == JSON_QUOTE_CHARACTER ) {
      inString = !inString;
    }

    if ( inString ) {
      _cursor.advance();
      continue;
    }

    if ( ch == JSON_OBJECT_START_CHARACTER || ch == JSON_ARRAY_START_CHARACTER ) {
      depth++;
    } else if ( ch == JSON_OBJECT_END_CHARACTER || ch == JSON_ARRAY_END_CHARACTER ) {
      if ( depth == 0 ) { break; }
      depth--;
    } else if ( ch == JSON_COMMA_CHARACTER && depth == 0 ) {
      break;
    }

    if ( depth < 0 ) break;

    _cursor.advance();
  }

  return ParseValueResult::OBJECT_PARSED;
}

// skip value for floating point : float or double
template <typename Cursor, bool UseMask> template <typename T, size_t N>
std::enable_if_t<N == 0 && std::is_floating_point_v<T>, ParseValueResult>
JSONParserBase<Cursor, UseMask>::skip_value() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_value floating point\n" );
  scan_char( '-', true );
  size_t len = scan_digits( MAX_VALUE_LENGTH );
  len += static_cast<uint8_t>( scan_char( '.', true ) );
  len += scan_digits( MAX_VALUE_LENGTH );

  return ( len > 0 ) ? ParseValueResult::FLOAT_PARSED
                     : ParseValueResult::NO_RESULT;
}

// skip value for unsigned integral
template <typename Cursor, bool UseMask> template <typename T, size_t N>
std::enable_if_t<N == 0 && std::is_integral_v<T> && std::is_unsigned_v<T>,
                 ParseValueResult>
JSONParserBase<Cursor, UseMask>::skip_value() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_value unsigned integral\n" );
  size_t len = scan_digits( MAX_VALUE_LENGTH );
  return ( len > 0 ) ? ParseValueResult::FLOAT_PARSED
                     : ParseValueResult::NO_RESULT;
}

// skip value for signed integral
template <typename Cursor, bool UseMask> template <typename T, size_t N>
std::enable_if_t<N == 0 && std::is_integral_v<T> && std::is_signed_v<T>,
                 ParseValueResult>
JSONParserBase<Cursor, UseMask>::skip_value() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_value signed integral\n" );
  scan_char( '-', true );
  size_t len = scan_digits( MAX_VALUE_LENGTH );
  return ( len > 0 ) ? ParseValueResult::FLOAT_PARSED
                     : ParseValueResult::NO_RESULT;
}

// skip value for string. We need to handle escape sequences.
template <typename Cursor, bool UseMask> template <typename T, size_t N>
std::enable_if_t<N == 0 && std::is_same_v<std::string_view, T>,
                 ParseValueResult>
JSONParserBase<Cursor, UseMask>::skip_value() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_value string\n" );
  if ( !scan_char( JSON_QUOTE_CHARACTER, true ) ) {
    return ParseValueResult::PARSE_ERROR_STRING_NO_START;
  }

  bool escape = false;
  bool inString = true;

  while ( inString ) {
    CHECK_LOOP( MAX_ITERATIONS, return ParseValueResult::PARSE_ERROR_OVERFLOW; );

    int c = _cursor.peek();

    if ( c < 0 ) { return ParseValueResult::PARSE_ERROR_STRING_NO_END; }

    char ch = static_cast<char>( c );

    if ( escape ) { escape = false; }

    if ( ch == JSON_ESCAPE_CHARACTER && !escape ) {
      escape = true;
      _cursor.advance();
      continue;
    }

    if ( ch == JSON_QUOTE_CHARACTER && !escape ) {
      inString = false;
      break;
    }

    _cursor.advance();
  }

  if ( !scan_char( JSON_QUOTE_CHARACTER, true ) ) {
    return ParseValueResult::PARSE_ERROR_STRING_NO_END;
  }

  return ParseValueResult::STRING_PARSED;
}

// template <typename Cursor, bool UseMask>
// template <typename T, size_t N>
// std::enable_if_t<is_array_of_basic_values<T>, ParseValueResult>
// JSONParserBase<Cursor, UseMask>::skip_value() {
//   JSON_DEBUG_INFO("JSONParserBase::skip_value array of basic values\n");
//   return skip_to_array_end_fast<typename container_info<T>::base_type>();
// }

// skip value for boolean
template <typename Cursor, bool UseMask> template <typename T, size_t N>
std::enable_if_t<N == 0 && std::is_same_v<T, bool>, ParseValueResult>
JSONParserBase<Cursor, UseMask>::skip_value() {
  return ( scan_keyword( JSON_TRUE ) ||
           scan_keyword( JSON_FALSE ) );
}

template <typename Cursor, bool UseMask>
ParseValueResult JSONParserBase<Cursor, UseMask>::skip_to_object_end() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_to_object_end\n" );
  // We are in the middle of an object before the comma, we need to skip to the
  // end of the object We use skip_value to skip the each value until
  // we find the end of the object

  while ( true ) {
    CHECK_LOOP( MAX_ITERATIONS, return ParseValueResult::PARSE_ERROR_OVERFLOW; );

    if ( is_object_end() ) { break; }

    ParseValueResult r = skip_value();

    if ( r ) {
      if ( is_object_end() ) {
        break;
      } else {
        SKIP_SPACES

        if ( !scan_char( JSON_COMMA_CHARACTER, true ) ) {
          JSON_DEBUG_ERROR( "JSONParserBase::skip_to_object_end: no comma\n" );
          return ParseValueResult::PARSE_ERROR_OBJECT_NO_COMMA;
        }
      }

      continue;
    } else {
      JSON_DEBUG_ERROR(
          "JSONParserBase::skip_to_object_end: cannot parse value "
          "%s for key #%zu\n",
          errorToString( r ),
          GLOBAL_ITERATIONS );
      return r;
    }
  }

  return ParseValueResult::OBJECT_PARSED;
}

template <typename Cursor, bool UseMask> template <typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::skip_to_array_end() {
  JSON_DEBUG_INFO( "JSONParserBase::skip_to_array_end typed\n" );
  // JSON_DEBUG_INFO("is container %d ", container_info<V>::is_container);
  // JSON_DEBUG_INFO("dimensions %d ", container_info<V>::dimensions);
  // JSON_DEBUG_INFO("is basic value %d ", is_basic_value<V>);
  // JSON_DEBUG_INFO("is array of basic values %d\n",
  // is_array_of_basic_values<V>);

  if constexpr ( container_info<V>::is_container &&
                 is_basic_value<typename container_info<V>::base_t> ) {
    return skip_to_array_end_fast<V>();
  }

  // We are in the middle of an array after the comma, we need to skip to the
  // end of the array We use skip_value to skip the each value until we find the
  // end of the array;
  SKIP_SPACES

  while ( true ) {
    CHECK_LOOP( MAX_ITERATIONS, return ParseValueResult::PARSE_ERROR_OVERFLOW; );

    // Always use the generic skip_value() here.
    ParseValueResult r = skip_value();

    if ( r ) {
      if ( is_array_end() ) {
        _cursor.advance();
        break;
      } else {
        // skip the comma
        SKIP_SPACES

        if ( !scan_char( JSON_COMMA_CHARACTER, true ) ) {
          JSON_DEBUG_ERROR( "JSONParserBase::skip_to_array_end: no comma\n" );
          return ParseValueResult::NO_RESULT;
        }
        continue;
      }

    } else {
      return ParseValueResult::NO_RESULT;
    }
  }

  return ParseValueResult::ARRAY_PARSED;
}

// Skip to end of multidimensional arrays with basic leaf value (e.g. a value we
// know it does not contain brackets) like [1,2,3,4] or [[1.1,2.2], [1.1,2.2],
// [1.1,2.2]] or [[[1.1,2.2], [1.1,2.2], [1.1,2.2]], [[1.1,2.2], [1.1,2.2],
// [1.1,2.2]]]
template <typename Cursor, bool UseMask> template <typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::skip_to_array_end_fast() {
  JSON_DEBUG_COLOR( COLOR_GREEN, "JSONParserBase::skip_to_array_end_fast\n" );
  // constexpr dimension = container_info<V>::dimensions;
  // constexpr base_type = typename container_info<V>::base_type;
  size_t brackets_counter = 1;
  // We are in the middle of an array after the comma, we need to skip to the
  // end of the array We just go to the end of the array assuming none of the
  // values contains ']'.
  SKIP_SPACES

  while ( brackets_counter > 0 ) {
    CHECK_LOOP( MAX_ITERATIONS, return ParseValueResult::PARSE_ERROR_OVERFLOW; );

    if constexpr ( container_info<V>::dimensions == 1 ) {
      if ( is_array_end() ) { brackets_counter--; }
    } else {
      if ( is_array_start() ) {
        brackets_counter++;
      } else if ( is_array_end() ) {
        brackets_counter--;
      }
    }

    _cursor.advance();

    if ( _cursor.eof() ) {
      JSON_DEBUG_ERROR(
          "JSONParserBase::skip_to_array_end_fast: no array end\n" );
      return ParseValueResult::PARSE_ERROR_ARRAY_NO_END;
    }
  }

  SKIP_SPACES

  return ParseValueResult::ARRAY_PARSED;
}

// ============================================================
//
//                    ASSIGNMENT
//
// ============================================================

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_parsed_value_to_value( PV& pv, V& v ) {
  JSON_DEBUG_TYPES( "Assign %s to %s\n", pv, v );
  ParseValueResult result = ParseValueResult::NO_RESULT;
  /*if constexpr (std::is_same_v<PV, V> &&
                is_container_from_list<V, arguments_array_types>::value &&
                container_info<V>::dimensions == 1) {
    result |= assign_array_to_array(pv, v) | ParseValueResult::VALUE_CONVERTED;
  } else */
  if constexpr ( std::is_same_v<PV, V> ) {
    result |= assign_same_type( pv, v ) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_convertible_v<PV, V> &&
                        std::is_integral_v<PV> && std::is_integral_v<V> ) {
    result |= assign_integral_to_integral( pv, v ) |
              ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_convertible_v<PV, V> &&
                        std::is_floating_point_v<PV> ) {
    result |= assign_convertible( pv, v ) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_same_v<PV, std::string_view> &&
                        is_char_array_v<V> ) {
    result |= assign_string_view_to_char_array( pv, v ) |
              ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_same_v<PV, NullType> && std::is_pointer_v<V> ) {
    result |=
        assign_null_ptr_to_pointer( pv, v ) | ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_same_v<PV, NaNType> ) {
    return result;
  } else if constexpr ( std::is_same_v<PV, InfinityType> ) {
    result |= assign_infinity_to_integral( pv, v ) |
              ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_same_v<PV, std::string_view> &&
                        is_uint_array_v<V> ) {
    result |= assign_string_view_to_unsigned_array( pv, v ) |
              ParseValueResult::VALUE_CONVERTED;
  } else if constexpr ( std::is_same_v<V, JSONCallbackObject> ) {
    result |=
        assign_callback_object( pv, v ) | ParseValueResult::VALUE_CONVERTED;
  } else {
    result |= assign_not_handled( pv, v );
  }
  return result;
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::assign_same_type( PV& pv,
                                                                    V& v ) {
  return assign<UseMask>( pv, v );
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult JSONParserBase<Cursor, UseMask>::assign_convertible( PV& pv,
                                                                      V& v ) {

  if ( v != pv ) {
    v = static_cast<V>( pv );
    return ParseValueResult::VALUE_UPDATED;
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_integral_to_integral( PV& pv, V& v ) {
#ifndef JSON_STRICT_MODE
  if constexpr ( ALLOW_INTEGER_OVERFLOW ) {
    pv = clamp_to_min_max<PV, V>( pv );
  }
#endif
  return assign<UseMask>( pv, v );
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_infinity_to_integral( PV&, V& v ) {
  if constexpr ( std::is_integral_v<V> ) {
    V nv = std::numeric_limits<V>::max();
    return assign<UseMask>( nv, v );
  }
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_string_view_to_char_array( PV& pv,
                                                                   V& v ) {
  if ( memcmp( v, pv.data(), pv.length() ) == 0 ) {
    return ParseValueResult::NO_RESULT;
  }

  size_t len = std::min( pv.length(), sizeof( v ) - 1 );
  std::memcpy( v, pv.data(), len );
  v[len] = '\0';

  return ParseValueResult::VALUE_UPDATED;
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_null_ptr_to_pointer( PV&, V& v ) {
  return assign<UseMask>( nullptr, v );
}

// template <typename Cursor, bool UseMask>
// template <typename V>
// ParseValueResult
// JSONParserBase<Cursor, UseMask>::assign_array_to_array(V& pv, V& v) {
//   return copy_array(v, pv) ? ParseValueResult::VALUE_UPDATED
//                            : ParseValueResult::NO_RESULT;
// }

template <typename Cursor, bool UseMask> template <typename V> ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_string_view_to_unsigned_array(
    std::string_view& pv, V& v ) {
  return copy_hex_be_to_h( v, pv.data(), pv.length() )
             ? ParseValueResult::VALUE_UPDATED
             : ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask> template <typename PV>
ParseValueResult JSONParserBase<Cursor, UseMask>::assign_callback_object(
    const PV& pv, JSONCallbackObject& cb ) {
  cb.run( pv );

  switch ( cb.skip ) {
    case JSON::SKIP::END:
      _state = SKIP;
      break;
    case JSON::SKIP::STOP:
      _state = STOPPED;
      break;
    default:
      break;
  }

  return ParseValueResult::VALUE_UPDATED;
}

template <typename Cursor, bool UseMask> template <typename PV, typename V>
ParseValueResult
JSONParserBase<Cursor, UseMask>::assign_not_handled([[maybe_unused]] PV& pv,[[maybe_unused]] V& v ) {
  JSON_DEBUG_TYPES( "Could not assign value from %s to %s\n", pv, v );
  return ParseValueResult::NO_RESULT;
}

template <typename Cursor, bool UseMask>
void JSONParserBase<Cursor, UseMask>::set_state( ParserState s ) {
  if ( /*_state == END ||*/ _state == ERROR || _state == STOPPED ) return;

  _state = s;

  if ( _state == COMMA ) { _reset_key(); }
}

template <typename Cursor, bool UseMask>
void JSONParserBase<Cursor, UseMask>::_reset_key() {
  _s_key_buf[0] = '\0';
  _key_length = 0;
}

template <typename Cursor, bool UseMask> template <class From, class To>
constexpr To JSONParserBase<Cursor, UseMask>::clamp_to_min_max( From v ) {
  if constexpr ( std::is_signed_v<From> && std::is_unsigned_v<To> ) {
    if ( v < 0 ) return 0;
  }
  // Upper-bound comparison is only safe when To::max fits in From without
  // overflow.  That holds when sizeof(To) < sizeof(From), or when they are
  // the same size and neither is signed→unsigned (which would make
  // To::max > From::max and overflow the cast).
  constexpr bool upper_safe =
      ( sizeof( To ) < sizeof( From ) ) ||
      ( sizeof( To ) == sizeof( From ) &&
        !(std::is_signed_v<From> && std::is_unsigned_v<To>));
  if constexpr ( upper_safe ) {
    if ( static_cast<From>( std::numeric_limits<To>::max() ) < v )
      return std::numeric_limits<To>::max();
  }
  // Lower-bound comparison is only meaningful for signed To and only safe
  // when To::min fits in From (i.e. sizeof(To) < sizeof(From) for signed).
  if constexpr ( std::is_signed_v<To> && std::is_signed_v<From> &&
                 sizeof( To ) < sizeof( From ) ) {
    if ( static_cast<From>( std::numeric_limits<To>::min() ) > v )
      return std::numeric_limits<To>::min();
  }
  return static_cast<To>( v );
}
// ==============================================================
//                      SCANING
// ==============================================================

// ── scan_digits ───────────────────────────────────────────────
template <typename Cursor, bool UseMask>
size_t JSONParserBase<Cursor, UseMask>::scan_digits( size_t max_length ) {
#if defined( __clang__ )
  __builtin_assume( max_length <= JSON::MAX_VALUE_LENGTH );
#endif
  return scan_ranges( JSON_DIGIT_CHARACTERS_RANGES, max_length, true );
}

// --- scan_char ---
template <typename Cursor, bool UseMask>
bool JSONParserBase<Cursor, UseMask>::scan_char( char c, bool consume ) {
  int got = _cursor.peek();
  if ( got < 0 || static_cast<char>( got ) != c ) return false;
  if ( consume ) _cursor.advance();

  return true;
}

// inline void swap_chars(char* arr, size_t index1, size_t index2) {
//     char temp = arr[index1];
//     arr[index1] = arr[index2];
//     arr[index2] = temp;
// }

template <typename Cursor, bool UseMask>
void JSONParserBase<Cursor, UseMask>::skip_spaces() {

  while ( true ) {
    CHECK_LOOP(MAX_ITERATIONS, std::printf("JSONParserBase::skip_spaces: too many iterations\n"); return; );
    
    int c = _cursor.peek();
    if ( c < 0 ) break;
    
    // if ( (SPACE_CHARACTERS_COMMON_LOW &~(static_cast<unsigned char>( c ))) == 0 ) break;
    char ch = static_cast<unsigned char>( c );
    if ((SPACE_CHARACTERS_COMMON_LOW &~(ch)) != SPACE_CHARACTERS_COMMON_LOW || (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')) break;

    _cursor.read(); // This will force a refill if needed
  }
}

// --- scan_keyword ---

template <typename Cursor, bool UseMask> template <size_t KwN> bool
JSONParserBase<Cursor, UseMask>::scan_keyword( const char ( &keyword )[KwN]) {

  uint8_t i = 0;
  for (; i < KwN; i++ ) {
    int c = _cursor.read();
    
    if ( c < 0 || static_cast<char>( c ) != keyword[i] ) {
      JSON_DEBUG_WARNING( "JSONParserBase::scan_keyword failed at %d parsed=%c keyword=%c\n", i, c, keyword[i]);
      _cursor.advance(-i - 1);
      return false;
    }
  }

  return true;
}

// --- scan_ranges_once : teste un seul caractère contre N plages ---
template <typename Cursor, bool UseMask> template <size_t RN> constexpr bool
JSONParserBase<Cursor, UseMask>::scan_ranges_once( char ( &ranges )[RN][2],
                                                   bool consume ) {
  int got = _cursor.peek();
  if ( got < 0 ) return false;
  char c = static_cast<char>( got );
  for ( uint8_t i = 0; i < RN; i++ ) {
    if ( c >= ranges[i][0] && c <= ranges[i][1] ) {
      if ( consume ) _cursor.advance();
      return true;
    }
  }
  return false;
}

// --- scan_ranges : avance tant que les caractères sont dans les plages ---
template <typename Cursor, bool UseMask> template <size_t RN> constexpr bool
JSONParserBase<Cursor, UseMask>::scan_ranges( char ( &ranges )[RN][2],
                                              size_t maxLen,
                                              bool consume ) {
  int n = 0;
  size_t iteration = 0;
  while ( ( maxLen == 0 || n < static_cast<int>( maxLen ) ) &&
          ++iteration < JSON::MAX_ITERATIONS ) {

    int got = _cursor.peek( n );
    if ( got < 0 ) break;
    char c = static_cast<char>( got );
    bool matched = false;
    for ( uint8_t i = 0; i < RN; i++ ) {
      if ( c >= ranges[i][0] && c <= ranges[i][1] ) {
        matched = true;
        break;
      }
    }
    if ( !matched ) break;
    n++;
  }
  bool result = ( n > 0 );
  if ( consume ) _cursor.advance( n );
  return result;
}

// ============================================================
//                     UTILS
// ============================================================

#if JSON_DEBUG_LEVEL > 0
#define DEBUG_OFFSET 30
template <typename Cursor, bool UseMask>
void JSONParserBase<Cursor, UseMask>::print_state( size_t iteration ) {
    if (_cursor.bytesConsumed() == 0) {
      std::printf("\x1b[32mSTART\x1b[0m\n");
      return;
    }

    [[maybe_unused]] const char* color = ( _state == ERROR ) ? "\x1b[31m" : "\x1b[32m";
    char errors[128] = {0};
    if (_lastError != ParserError::NO_ERROR) {
      snprintf(errors, sizeof(errors), "error=%s ", errorToString(_lastError));
    }
    if (_lastParseError != ParseValueResult::NO_RESULT) {
      snprintf(errors, sizeof(errors), "parseError=%s ", errorToString(_lastParseError));
    }

    char json[DEBUG_OFFSET * 2 + 1];
    const char* begin = std::max(_cursor.ptr() - DEBUG_OFFSET, _cursor.start());
    const char* end = std::min(_cursor.ptr() + DEBUG_OFFSET, _cursor.start() + _cursor.size());
    size_t json_size = end - begin;
    [[maybe_unused]] int offset = _cursor.ptr() - begin;
    strncpy( json, begin, json_size );

    replace_endl( json, sizeof( json ));
    [[maybe_unused]] std::string_view state = get_state_name();
    JSON_DEBUG_ERROR( "'%.*s'\n%s%*c[%zu]:%.*s %skey='%.*s' \x1b[0m\n",
                     json_size,
                     json,
                     color,
                     ( 1 + offset ),
                     '^',
                     _cursor.bytesConsumed(),
                     (int)state.length(),
                     state.data(),
                     errors,
                     (int)_key_length,
                     _s_key_buf );
  //}
}

template <typename Cursor, bool UseMask>
std::string_view JSONParserBase<Cursor, UseMask>::get_state_name() {
  switch ( _state ) {
    case START:
      return "START";
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
    case SKIP:
      return "SKIP";
      break;
    default:
      return "UNKNOWN";
  }
}
#endif