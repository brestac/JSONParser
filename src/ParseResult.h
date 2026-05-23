#pragma once

#include "macros.h"
#include <stddef.h>
#include <stdint.h>

NAMESPACE_JSON_BEGIN

struct ParseResult {
  size_t length;
  size_t nKeys;
  size_t nParsed;
  size_t nConverted;
  size_t nUpdated;
  uint8_t error;
  uint64_t elapsed;
  bool stopped;

  ParseResult() : length(0), nKeys(0), nParsed(0), nConverted(0), nUpdated(0), error(0), elapsed(0), stopped(false) {}

  template <typename T> ParseResult(T *parser, uint64_t duration) {
    length = parser->parsed_length();
    nKeys = parser->nKeys();
    nParsed = parser->nParsed();
    nConverted = parser->nConverted();
    nUpdated = parser->nUpdated();
    error = parser->error();
    elapsed = duration;
    stopped = parser->stopped();
  }

  ParseResult(size_t length, size_t nKeys, size_t nParsed, size_t nConverted, size_t nUpdated, uint8_t error,
              uint64_t elapsed, bool stopped = false)
      : length(length), nKeys(nKeys), nParsed(nParsed), nConverted(nConverted), nUpdated(nUpdated), error(error),
        elapsed(elapsed), stopped(stopped) {}

  operator size_t() const { return length; }

  void print();
};

void ParseResult::print() {
  DEBUG_PRINTF("{\"length\":%zu,\"nKeys\":%zu,\"nParsed\":%zu,\"nMatched\":%zu,"
               "\"nUpdated\":%zu,\"error\":%hhu,\"elapsed\":%lu,\"stopped\":%d}\n",
               length, nKeys, nParsed, nConverted, nUpdated, error, elapsed, stopped);
}

NAMESPACE_JSON_END
