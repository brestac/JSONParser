#pragma once

#include <stddef.h>
#include <stdint.h>

#include "macros.h"
#include "ParseValueResult.h"

NAMESPACE_JSON_BEGIN

enum ParserError : uint8_t {
  NO_ERROR = 0,
  NO_OBJECT_START = 1,
  INVALID_KEY = 2,
  NO_COLON = 3,
  INVALID_VALUE = 4,
  NO_COMMA = 5,
  INVALID_OBJECT = 6,
  TOO_MANY_ITERATIONS = 7,
  MAX_DEPTH_REACHED = 8,
  MALFORMED_JSON = 9
};

static const char* errorToString(ParserError& error) {
  switch (error) {
  case ParserError::NO_ERROR:
    return "NO ERROR";
  case ParserError::NO_OBJECT_START:
    return "NO OBJECT START";
  case ParserError::INVALID_KEY:
    return "INVALID KEY";
  case ParserError::NO_COLON:
    return "NO COLON";
  case ParserError::INVALID_VALUE:
    return "INVALID VALUE";
  case ParserError::NO_COMMA:
    return "NO COMMA";
  case ParserError::INVALID_OBJECT:
    return "INVALID OBJECT";
  default:
    return "UNKNOWN";
  }
}

struct ParseResult {
  size_t length;
  uint8_t nParsed;
  uint8_t nMatched;
  uint8_t nConverted;
  uint8_t nUpdated;
  ParserError error;
  ParseValueResult parseError;
  uint64_t elapsed;
  bool stopped;

  ParseResult() : length(0), nParsed(0), nMatched(0), nConverted(0), nUpdated(0), error(ParserError::NO_ERROR), parseError(ParseValueResult()), elapsed(0), stopped(false) {}

  template <typename T> ParseResult(T *parser, uint64_t duration) {
    length = parser->bytesConsumed();
    nParsed = parser->nParsed();
    nMatched = parser->nMatched();
    nConverted = parser->nConverted();
    nUpdated = parser->nUpdated();
    error = parser->error();
    parseError = parser->parseError();
    elapsed = duration;
    stopped = parser->stopped();
  }

  ParseResult(size_t length, size_t /*nKeys*/, uint8_t nParsed, uint8_t nMatched, uint8_t nConverted, uint8_t nUpdated, ParserError error, ParseValueResult parseError,
              uint64_t elapsed, bool stopped = false)
      : length(length), nParsed(nParsed), nMatched(nMatched), nConverted(nConverted), nUpdated(nUpdated), error(error), parseError(parseError),
        elapsed(elapsed), stopped(stopped) {}

  operator size_t() const { return length; }

  void print();
};

void ParseResult::print() {
  DEBUG_PRINTF("{\"length\":%zu,\"nParsed\":%zu,\"nMatched\":%zu,\"nConverted\":%zu,"
               "\"nUpdated\":%zu,\"error\":\"%s\",\"parseError\":\"%s\",\"elapsed\":%lu,\"stopped\":%s}\n",
               length, nParsed, nMatched, nConverted, nUpdated, errorToString(error), errorToString(parseError), elapsed, stopped ? "true" : "false");
}

NAMESPACE_JSON_END
