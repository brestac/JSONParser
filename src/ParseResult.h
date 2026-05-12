#pragma once

#include <stdint.h>
#include <stddef.h>
#include "macros.h"

namespace JSON {
  struct ParseResult {
    size_t length;
    size_t nKeys;
    size_t nParsed;
    size_t nConverted;
    size_t nUpdated;
    bool error;
    uint64_t elapsed;
    bool stopped;

    ParseResult() : length(0), nKeys(0), nParsed(0), nConverted(0), nUpdated(0), error(false), elapsed(0), stopped(false) {}
    ParseResult(size_t length, size_t nKeys, size_t nParsed, size_t nConverted, size_t nUpdated, bool error, uint64_t elapsed, bool stopped = false) : length(length), nKeys(nKeys), nParsed(nParsed), nConverted(nConverted), nUpdated(nUpdated), error(error), elapsed(elapsed), stopped(stopped) {}
    operator size_t() const { return length; }

    void print();
  };

  void ParseResult::print() {
    DEBUG_PRINTF("{\"length\":%zu,\"nKeys\":%zu,\"nParsed\":%zu,\"nMatched\":%zu,\"nUpdated\":%zu,\"error\":%s,\"elapsed\":%lu, \"stopped\":%d}\n", length, nKeys, nParsed, nConverted, nUpdated, error ? "true" : "false", elapsed, stopped);
  }
}
