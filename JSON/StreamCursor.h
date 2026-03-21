#pragma once

#include "constants.h"
#include <stream.h>
#include "macros.h"

NAMESPACE_JSON_BEGIN

// ============================================================
//  RingBuffer<N>
//  Buffer circulaire de taille N (doit être une puissance de 2).
//  Se remplit à la demande depuis un Stream Arduino.
// ============================================================

template <size_t N>
class RingBuffer {
    static_assert(N >= 16,            "RingBuffer : N doit être >= 16");
    static_assert((N & (N - 1)) == 0, "RingBuffer : N doit être une puissance de 2");

public:
    explicit RingBuffer(Stream &stream)
        : _stream(stream), _head(0), _tail(0) {}

    // Nombre d'octets disponibles en lecture sans refill
    size_t available() const { return _head - _tail; }

    // Tente de remplir le buffer depuis le stream (appels non-bloquants).
    // Ne lit que les octets immédiatement disponibles.
    void refill() {
        while (available() < N) {
            int avail = _stream.available();
            if (avail <= 0) break;
            int c = _stream.read();
            if (c < 0) break;
            _buf[_head++ & MASK] = static_cast<char>(c);
        }
    }

    // Peek à l'offset i (0 = prochain octet), sans consommer.
    // Effectue un refill si nécessaire.
    // Retourne -1 si la donnée n'est pas disponible (timeout / fin de flux).
    int peek(size_t offset = 0) {
        if (offset >= available()) refill();
        if (offset >= available()) return -1;
        return static_cast<unsigned char>(_buf[(_tail + offset) & MASK]);
    }

    // Consomme n octets (les marque comme lus)
    void consume(size_t n) {
        _tail += n;
    }

    // Lit et consomme un octet. Retourne -1 si vide.
    int read() {
        if (available() == 0) refill();
        if (available() == 0) return -1;
        return static_cast<unsigned char>(_buf[_tail++ & MASK]);
    }

private:
    static constexpr size_t MASK = N - 1;
    Stream &_stream;
    char    _buf[N];
    size_t  _head;   // indice d'écriture absolu
    size_t  _tail;   // indice de lecture absolu
};

// ============================================================
//  StreamCursor<N>
//  Curseur de lecture sur un RingBuffer.
//  Remplace const char*& dans toutes les fonctions scan_*.
//
//  N : taille du ring buffer (puissance de 2, >= 16).
//      Doit être > au plus long token JSON attendu (max ~255 pour une clé,
//      mais en pratique 64–512 suffisent pour les tokens structurels).
//      Pour les valeurs de chaînes longues, scan_until() les lit
//      caractère par caractère sans tout mettre en RAM.
// ============================================================
class StreamCursor {
public:
    explicit StreamCursor(Stream &stream)
        : _ring(stream), _consumed(0), _eof(false) {}

    // --- Interface principale ---

    // Caractère courant sans avancer (-1 = fin de flux)
    int peek(size_t offset = 0) {
        int c = _ring.peek(offset);
        if (c < 0) _eof = true;
        return c;
    }

    // Avance d'un cran
    void advance(size_t n = 1) {
        _ring.consume(n);
        _consumed += n;
    }

    // Lit et avance d'un cran
    int read() {
        int c = _ring.read();
        if (c >= 0) _consumed++;
        else        _eof = true;
        return c;
    }

    bool eof() const { return _eof; }

    // Nombre total d'octets consommés depuis la création du curseur
    size_t bytesConsumed() const { return _consumed; }

    // Extrait au plus maxLen octets dans out[] en s'arrêtant sur l'un
    // des caractères délimiteurs JSON (séparateurs de valeur).
    // Utilisé pour isoler un token numérique avant strtod/strtol.
    // Ne consomme PAS les octets (lecture seule via peek).
    // Retourne le nombre d'octets copiés.
    size_t peekToken(char *out, size_t maxLen) {
        static const char delimiters[] = { ',', '}', ']', ' ', '\t', '\n', '\r', '\0' };
        size_t n = 0;
        while (n < maxLen) {
            int c = _ring.peek(n);
            if (c < 0) break;
            char ch = static_cast<char>(c);
            bool isDelim = false;
            for (char d : delimiters) {
                if (ch == d) { isDelim = true; break; }
            }
            if (isDelim) break;
            out[n++] = ch;
        }
        if (n < maxLen) out[n] = '\0';
        return n;
    }

private:
    RingBuffer<JSON::RingBufferSize> _ring;
    size_t        _consumed;
    bool          _eof;
};

NAMESPACE_JSON_END