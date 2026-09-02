#pragma once

// ---------------------------------------------------------------------------
//  ParseDispatchTable.h
//
//  Table de dispatch simplifiée : hash + arg_index uniquement.
//  Pas de DispatchFn — le dispatch se fait dans parse_value via
//  dispatch_by_index qui parcourt les indices compile-time.
//
//  Cycle de vie :
//    - SimpleDispatchTable : static local dans parse<Args...>
//                            construite UNE FOIS par spécialisation template
//                            (= une fois par combinaison unique d'arguments)
//    - TupleT refs         : local dans parse<Args...>
//                            reconstruit à chaque appel (nouvelles références)
// ---------------------------------------------------------------------------

#include "JSONKey.h"
#include "ParseValueResult.h"

// ---------------------------------------------------------------------------
//  SimpleEntry
// ---------------------------------------------------------------------------

struct SimpleEntry {
    uint32_t hash      = 0;
    size_t   arg_index = 0;   // index de la paire pour dispatch et keyMask automask
    int      key_index = -1;  // JSONKey::_index pour keyMask non-automask
};

// ---------------------------------------------------------------------------
//  SimpleDispatchTable<TupleT, NPairs>
//  Construite depuis les clés aux positions paires de TupleT.
// ---------------------------------------------------------------------------

// Nouvelle structure : uniquement les données statiques
struct StaticEntry {
    uint32_t hash      = 0;
    int      key_index = -1;
    size_t   arg_index = 0;
};

template <size_t NPairs>
struct StaticDispatchTable {
    std::array<StaticEntry, NPairs> entries{};

    template <size_t... I, typename TupleT>
    constexpr void fill(const TupleT& t, std::index_sequence<I...>) {
        ((entries[I] = StaticEntry{
            hash32(extract_key(std::get<I * 2>(t)).data(),
                   extract_key(std::get<I * 2>(t)).length()),
            extract_index(std::get<I * 2>(t)),
            I
        }), ...);
    }

    // Recherche linéaire par hash — suffisant pour < 32 champs
    const StaticEntry* find(uint32_t h) const {
        for (size_t i = 0; i < NPairs; ++i)
            if (entries[i].hash == h) return &entries[i];
        return nullptr;
    }
    
    template <typename TupleT>
    constexpr StaticDispatchTable(const TupleT& t) {
        fill(t, std::make_index_sequence<NPairs>{});
    }
};

// ---------------------------------------------------------------------------
//  call_at<I> — appelle parse_into_value si I == target
//  Retourne true si l'appel a eu lieu (pour le short-circuit du fold).
// ---------------------------------------------------------------------------

template <typename TargetT, size_t I, typename ParserT, typename TupleT>
bool call_at(size_t target_index, ParserT& parser, TupleT& refs, ParseValueResult& result) {
    if (I != target_index) return false;
    result = parser.template parse_into_value<TargetT>(std::get<I * 2 + 1>(refs));
    return true;
}

// ---------------------------------------------------------------------------
//  dispatch_by_index — fold expression sur 0..NPairs-1
//  Le || assure le short-circuit : dès que call_at retourne true, on s'arrête.
// ---------------------------------------------------------------------------

template <typename TargetT, typename ParserT, typename TupleT, size_t... I>
ParseValueResult dispatch_by_index(
        size_t target_index, ParserT& parser, TupleT& refs,
        std::index_sequence<I...>) {
    ParseValueResult result;
    (call_at<TargetT, I>(target_index, parser, refs, result) || ...);
    return result;
}