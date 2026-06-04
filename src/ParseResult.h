#pragma once

#include "macros.h"
#include <stddef.h>
#include <stdint.h>

NAMESPACE_JSON_BEGIN

enum ParserError : uint8_t {
  NO_ERROR = 0,
  NO_OBJECT_START = 1,
  INVALID_KEY = 2,
  NO_COLON = 3,
  INVALID_VALUE = 4,
  NO_COMMA = 5,
  INVALID_OBJECT = 6
};

static const char* errorToString(ParserError error) {
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
  size_t nParsed;
  size_t nMatched;
  size_t nConverted;
  size_t nUpdated;
  uint8_t error;
  uint64_t elapsed;
  bool stopped;

  ParseResult() : length(0), nParsed(0), nMatched(0), nConverted(0), nUpdated(0), error(0), elapsed(0), stopped(false) {}

  template <typename T> ParseResult(T *parser, uint64_t duration) {
    length = parser->parsed_length();
    nParsed = parser->nParsed();
    nMatched = parser->nMatched();
    nConverted = parser->nConverted();
    nUpdated = parser->nUpdated();
    error = parser->error();
    elapsed = duration;
    stopped = parser->stopped();
  }

  ParseResult(size_t length, size_t nKeys, size_t nParsed, size_t nMatched, size_t nConverted, size_t nUpdated, uint8_t error,
              uint64_t elapsed, bool stopped = false)
      : length(length), nParsed(nParsed), nMatched(nMatched), nConverted(nConverted), nUpdated(nUpdated), error(error),
        elapsed(elapsed), stopped(stopped) {}

  operator size_t() const { return length; }

  void print();
};

void ParseResult::print() {
  DEBUG_PRINTF("{\"length\":%zu,\"nParsed\":%zu,\"nMatched\":%zu,\"nMatched\":%zu,"
               "\"nUpdated\":%zu,\"error\":%hhu,\"elapsed\":%lu,\"stopped\":%d}\n",
               length, nParsed, nMatched, nConverted, nUpdated, error, elapsed, stopped);
}

NAMESPACE_JSON_END
