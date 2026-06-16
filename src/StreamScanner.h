#pragma once

// StreamScanner.h

#include "PointerCursor.h"
#include "StreamCursor.h"
#include "constants.h"

NAMESPACE_JSON_BEGIN

// --- scan_char ---
template <typename Cursor> bool cursor_scan_char(Cursor &cur, char c, bool consume = true) {
  int got = cur.peek();
  if (got < 0 || static_cast<char>(got) != c)
    return false;
  if (consume)
    cur.advance();
  return true;
}

// --- scan_keyword ---
template <typename Cursor, size_t KwN>
bool cursor_scan_keyword(Cursor &cur, const char (&keyword)[KwN], bool consume = true) {
  for (size_t i = 0; i < KwN; i++) {
    int c = cur.peek(i);
    if (c < 0 || static_cast<char>(c) != keyword[i])
      return false;
  }
  if (consume)
    cur.advance(KwN);
  return true;
}

// --- scan_until : avance jusqu'au délimiteur (non inclus par défaut) ---
template <typename Cursor>
bool cursor_scan_until(Cursor &cur, char delim, size_t maxLen = 0, bool consume = true, bool consumeDelim = false) {
  size_t i = 0;
  while (true) {
    CHECK_LOOP(false);
    
    int c = cur.peek(i);
    if (c < 0)
      return false; // fin de flux
    if (static_cast<char>(c) == delim)
      break; // délimiteur trouvé
    if (maxLen > 0 && i >= maxLen) {
      if (consume)
        cur.advance(i);
      return false; // dépassement
    }
    i++;
  }
  bool result = (i > 0);
  if (consume) {
    cur.advance(i);
    if (consumeDelim)
      cur.advance();
  }
  return result;
}

// --- scan_ranges_once : teste un seul caractère contre N plages ---
template <typename Cursor, size_t RN>
constexpr bool cursor_scan_ranges_once(Cursor &cur, char (&ranges)[RN][2], bool consume = true) {
  int got = cur.peek();
  if (got < 0)
    return false;
  char c = static_cast<char>(got);
  for (size_t i = 0; i < RN; i++) {
    if (c >= ranges[i][0] && c <= ranges[i][1]) {
      if (consume)
        cur.advance();
      return true;
    }
  }
  return false;
}

// --- scan_ranges : avance tant que les caractères sont dans les plages ---
template <typename Cursor, size_t RN>
constexpr bool cursor_scan_ranges(Cursor &cur, char (&ranges)[RN][2], size_t maxLen = 0, bool consume = true) {
  size_t n = 0;
  size_t iteration = 0;
  while ((maxLen == 0 || n < maxLen) && ++iteration < JSON::MAX_ITERATIONS) {

    int got = cur.peek(n);
    if (got < 0)
      break;
    char c = static_cast<char>(got);
    bool matched = false;
    for (size_t i = 0; i < RN; i++) {
      if (c >= ranges[i][0] && c <= ranges[i][1]) {
        matched = true;
        break;
      }
    }
    if (!matched)
      break;
    n++;
  }
  bool result = (n > 0);
  if (consume)
    cur.advance(n);
  return result;
}

// --- scan_chars_once : teste un seul caractère contre un ensemble ---
template <typename Cursor, size_t ChN>
bool cursor_scan_chars_once(Cursor &cur, const char (&chars)[ChN], bool consume = true) {
  int got = cur.peek();
  if (got < 0)
    return false;
  char c = static_cast<char>(got);
  for (size_t i = 0; i < ChN; i++) {
    if (c == chars[i]) {
      if (consume)
        cur.advance();
      return true;
    }
  }
  return false;
}

// --- skip_spaces ---
template <typename Cursor> bool cursor_skip_spaces(Cursor &cur) {
  bool skipped = false;
  while (true) {
    CHECK_LOOP(false);
    int got = cur.peek();
    if (got < 0)
      break;
    char c = static_cast<char>(got);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      cur.advance();
      skipped = true;
    } else {
      break;
    }
  }
  return skipped;
}

NAMESPACE_JSON_END
