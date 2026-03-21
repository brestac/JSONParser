#pragma once

// StreamScanner.h
//
// Fournit :
//   - RingBuffer<N>   : buffer circulaire alimenté depuis un Stream Arduino
//   - StreamCursor<N> : curseur de lecture sur ce ring buffer,
//                       offrant la même interface que const char*& dans scanner.h
//   - PointerCursor   : wrapper autour de const char*& pour uniformiser l'interface
//   - Fonctions stream_scan_*() : équivalents des scan_*() de scanner.h
//
// Usage :
//   #include "StreamScanner.h"
//   JSON::StreamCursor<256> cursor(myWiFiClient);
//   JSONStreamParser parser(cursor);

#ifdef ARDUINO
#include <Stream.h>
#include <StreamCursor.h>
#endif

#include "constants.h"
//#include "scanner.h"
#include "PointerCursor.h"

NAMESPACE_JSON_BEGIN

// ============================================================
//  RingBuffer<N>
//  Buffer circulaire de taille N (doit être une puissance de 2).
//  Se remplit à la demande depuis un Stream Arduino.
// ============================================================


// ============================================================
//  Fonctions scan_* génériques sur Cursor
//  Équivalentes aux fonctions de scanner.h, mais opèrent sur
//  un Cursor (PointerCursor ou StreamCursor<N>) au lieu de
//  const char*&.
// ============================================================

// --- scan_char ---
template <typename Cursor>
bool cursor_scan_char(Cursor &cur, char c, bool include = true) {
    int got = cur.peek();
    if (got < 0 || static_cast<char>(got) != c) return false;
    if (include) cur.advance();
    return true;
}

// --- scan_keyword ---
template <typename Cursor, size_t KwN>
bool cursor_scan_keyword(Cursor &cur, const char (&keyword)[KwN],
                         bool include = true) {
    for (size_t i = 0; i < KwN; i++) {
        int c = cur.peek(i);
        if (c < 0 || static_cast<char>(c) != keyword[i]) return false;
    }
    if (include) cur.advance(KwN);
    return true;
}

// --- scan_until : avance jusqu'au délimiteur (non inclus par défaut) ---
template <typename Cursor>
bool cursor_scan_until(Cursor &cur, char delim,
                       size_t maxLen = 0, bool include = true,
                       bool includeDelim = false) {
    size_t i = 0;
    while (true) {
        int c = cur.peek(i);
        if (c < 0) return false;                              // fin de flux
        if (static_cast<char>(c) == delim) break;            // délimiteur trouvé
        if (maxLen > 0 && i >= maxLen) {
            if (include) cur.advance(i);
            return false;                                     // dépassement
        }
        i++;
    }
    bool result = (i > 0);
    if (include) {
        cur.advance(i);
        if (includeDelim) cur.advance();
    }
    return result;
}

// --- scan_ranges_once : teste un seul caractère contre N plages ---
template <typename Cursor, size_t RN>
constexpr bool cursor_scan_ranges_once(Cursor &cur, char (&ranges)[RN][2],
                             bool include = true) {
    int got = cur.peek();
    if (got < 0) return false;
    char c = static_cast<char>(got);
    for (size_t i = 0; i < RN; i++) {
        if (c >= ranges[i][0] && c <= ranges[i][1]) {
            if (include) cur.advance();
            return true;
        }
    }
    return false;
}

// --- scan_ranges : avance tant que les caractères sont dans les plages ---
template <typename Cursor, size_t RN>
constexpr  bool cursor_scan_ranges(Cursor &cur, char (&ranges)[RN][2],
                        size_t maxLen = 0, bool include = true) {
    size_t n = 0;
    while (maxLen == 0 || n < maxLen) {
        int got = cur.peek(n);
        if (got < 0) break;
        char c = static_cast<char>(got);
        bool matched = false;
        for (size_t i = 0; i < RN; i++) {
            if (c >= ranges[i][0] && c <= ranges[i][1]) {
                matched = true;
                break;
            }
        }
        if (!matched) break;
        n++;
    }
    bool result = (n > 0);
    if (include) cur.advance(n);
    return result;
}

// --- scan_chars_once : teste un seul caractère contre un ensemble ---
template <typename Cursor, size_t ChN>
bool cursor_scan_chars_once(Cursor &cur, const char (&chars)[ChN],
                            bool include = true) {
    int got = cur.peek();
    if (got < 0) return false;
    char c = static_cast<char>(got);
    for (size_t i = 0; i < ChN; i++) {
        if (c == chars[i]) {
            if (include) cur.advance();
            return true;
        }
    }
    return false;
}

// --- skip_spaces ---
template <typename Cursor>
bool cursor_skip_spaces(Cursor &cur) {
    bool skipped = false;
    while (true) {
        int got = cur.peek();
        if (got < 0) break;
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
